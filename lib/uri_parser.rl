#include "siphon/uri.h"

#include <string.h>
#include <assert.h>

%%{
	machine uri_parser;

	action mark           { mark = p; }
	action scheme         { SET(SCHEME, 0); }
	action host_ip4       { SET(HOST, 0); u->host = SP_HOST_IPV4; }
	action host_ip6       { SET(HOST, 0); u->host = mapped ? SP_HOST_IPV4_MAPPED : SP_HOST_IPV6; }
	action host_ip4_map   { mapped = true; }
	action host_ip_future { SET(HOST, 0); u->host = SP_HOST_IP_FUTURE; }
	action host_name      { SET(HOST, 0); u->host = SP_HOST_NAME; }
	action port           { SET(PORT, 0); }
	action incr_port      { u->port = u->port * 10 + (fc - '0'); }
	action path           { SET(PATH, 0); }
	action query          { SET(QUERY, 0); }
	action fragment       { SET(FRAGMENT, 0); }
	action mark_user      { user = p; }
	action mark_pass      { pass = p; }
	action userinfo       {
		size_t back;
		if (pass == NULL) {
			back = 0;
		}
		else {
			mark = pass;
			SET(PASSWORD, 0);
			back = p - pass + 1;
		}
		mark = user;
		SET(USER, back);
	}


	# IPv4 ##################################

	ip4_oct = digit              # 0-9
	        | "1".."9" digit     # 10-99
	        | "1" digit{2}       # 100-199
	        | "2" "0".."4" digit # 200-249
	        | "25" "0".."5"      # 250-255
	        ;

	ip4     = ( ip4_oct "." ip4_oct "." ip4_oct "." ip4_oct ) ;


	# IPv6 ##################################

	ip6_grp = xdigit{1,4};

	ip6_end = ip6_grp ":" ip6_grp
	        | ip4 %host_ip4_map
	        ;

	ip6     =                                      (ip6_grp ":"){6} ip6_end
	        |                                 "::" (ip6_grp ":"){5} ip6_end
	        | (                    ip6_grp )? "::" (ip6_grp ":"){4} ip6_end
	        | (( ip6_grp ":" ){,1} ip6_grp )? "::" (ip6_grp ":"){3} ip6_end
	        | (( ip6_grp ":" ){,2} ip6_grp )? "::" (ip6_grp ":"){2} ip6_end
	        | (( ip6_grp ":" ){,3} ip6_grp )? "::"  ip6_grp ":"     ip6_end
	        | (( ip6_grp ":" ){,4} ip6_grp )? "::"                  ip6_end
	        | (( ip6_grp ":" ){,5} ip6_grp )? "::"                  ip6_grp
	        | (( ip6_grp ":" ){,6} ip6_grp )? "::"
	        ;


	# URI ###################################

	uri_sub_delims  = "!" | "$" | "&" | "'" | "(" | ")" | "*" | "+" | "," | ";" | "=" ;
	uri_gen_delims  = ":" | "/" | "?" | "#" | "[" | "]" | "@" ;
	uri_reserved    = uri_gen_delims | uri_sub_delims ;
	uri_unreserved  = alnum | "-" | "." | "_" | "~" ;

	uri_pct_enc     = "%" xdigit xdigit ;

	uri_pchar_spec  = uri_unreserved | uri_pct_enc | uri_sub_delims | ":" | "@" | "|" ;
	uri_pchar_ext   = uri_pchar_spec | '"' | "<" | ">" | "\\" | "^" | "`" | "{" | "}" ;
	uri_pchar       = uri_pchar_ext ;
	uri_qchar       = uri_pchar | "[" | "]" | "/" | "?" ;
	uri_fchar       = uri_qchar | "#" ;

	uri_query       = uri_qchar* >mark %query ;
	uri_fragment    = uri_fchar* >mark %fragment ;
	uri_extra       = ( "?" uri_query )? ( "#" uri_fragment )? ;

	uri_seg         = uri_pchar* ;
	uri_seg_nz      = uri_pchar+ ;
	uri_seg_nz_nc   = ( uri_pchar - ":" )+ ;
	uri_seg_ext     = ( uri_pchar | "[" | "]" )* ;
	uri_seg_ext_nz  = ( uri_pchar | "[" | "]" )+ ;

	uri_path_abs    = ( "/" uri_seg_ext )* >mark %path ;
	uri_path_abs_nz = ( "/" ( uri_seg_ext_nz ( "/" uri_seg_ext )* )? ) >mark %path ;
	uri_path_rel_nc = ( uri_seg_nz_nc ( "/" uri_seg_ext )* ) >mark %path ;
	uri_path_rel    = ( uri_seg_nz ( "/" uri_seg_ext )* ) >mark %path ;
	uri_path_empty  = "" >mark %path ;

	ip_future       = "v" xdigit+ "." ( uri_unreserved | uri_sub_delims | ":" )+ ;

	uri_port        = digit* >mark @incr_port %port ;

	uri_ip_future   = ip6 >mark %host_ip6
	                | ip_future >mark %host_ip_future
	                ;

	uri_auth_val    = ( uri_unreserved | uri_pct_enc | uri_sub_delims )+ ;

	uri_host        = "[" uri_ip_future "]"
	                | ip4 >mark %host_ip4
	                | (uri_auth_val - ip4) >mark %host_name
	                | null
	                ;

	uri_userinfo    = ( uri_auth_val >mark_user )? ( ":" ( uri_auth_val >mark_pass ) )? "@" >userinfo ;
	uri_authority   = uri_userinfo? uri_host ( ":" uri_port )? ;

	uri_rel_part    = "//" uri_authority uri_path_abs
	                | uri_path_abs_nz
	                | uri_path_rel_nc
	                | uri_path_empty
	                ;

	uri_hier_part   = "//" uri_authority uri_path_abs
	                | uri_path_abs_nz
	                | uri_path_rel
	                | uri_path_empty
	                ;

	uri_scheme      = alpha >mark ( alpha | digit | "+" | "-" | "." )* %scheme ;

	uri_relative    = uri_rel_part uri_extra ;
	uri_absolute    = uri_scheme ":" uri_hier_part uri_extra ;
	uri             = uri_absolute | uri_relative;


	# Entry #################################

	main := uri;
}%%


ssize_t
sp_uri_parse (SpUri *u, const char *restrict buf, size_t len)
{
	assert (u != NULL);
	assert (buf != NULL);

	const char *restrict p = buf;
	const char *restrict pe = p + len;
	const char *restrict eof = pe;
	const char *restrict mark = p;
	const char *restrict user = NULL;
	const char *restrict pass = NULL;
	int cs = %%{ write start; }%%;
	bool mapped = false;

	memset (u->seg, 0, sizeof u->seg);
	u->port = 0;
	u->first = SP_URI_NONE;
	u->last = SP_URI_NONE;
	u->host = SP_HOST_NONE;

#define SET(name, back) do {              \
	if (u->first == SP_URI_NONE) {        \
		u->first = SP_URI_##name;         \
	}                                     \
	u->last = SP_URI_##name;              \
	u->seg[SP_URI_##name] = (SpRange16) { \
		mark-buf,                         \
		p-mark-back                       \
	};                                    \
} while (0)
	%% write exec;
#undef SET
 
	if (cs < %%{ write first_final; }%%) {
		return -1 - (p - buf);
	}
	else {
		return p - buf;
	}
}

