local ffi = require('ffi')
local lib = ffi.load('siphon')
local C = ffi.C

ffi.cdef[[
typedef union {
	// request line values
	struct {
		uint8_t method_off;
		uint8_t method_len;
		uint16_t uri_off;
		uint16_t uri_len;
		uint8_t version;
	} request;

	// response line values
	struct {
		uint16_t reason_off;
		uint16_t reason_len;
		uint16_t status;
		uint8_t version;
	} response;

	// header field name and value
	struct {
		uint16_t name_off;
		uint16_t name_len;
		uint16_t value_off;
		uint16_t value_len;
	} field;

	// beginning of body
	struct {
		size_t content_length;
		bool chunked;
	} body_start;

	// size of next chunk
	struct {
		size_t length;
	} body_chunk;
} SpHttpValue;

typedef enum {
	SP_HTTP_NONE = -1,
	SP_HTTP_REQUEST,     // complete request line
	SP_HTTP_RESPONSE,    // complete response line
	SP_HTTP_FIELD,       // header or trailer field name and value
	SP_HTTP_BODY_START,  // start of the body
	SP_HTTP_BODY_CHUNK,  // size for chunked body
	SP_HTTP_BODY_END,    // end of the body chunks
	SP_HTTP_TRAILER_END  // complete request or response
} SpHttpType;

typedef struct {
	SpHttpValue as;   // captured value
	SpHttpType type;  // type of the captured value
	unsigned cs;      // current scanner state
	size_t off;       // internal offset mark
	size_t body_len;  // content length or current chunk size
	uint16_t scans;   // number of passes through the scanner
	bool response;    // true if response, false if request
	bool chunked;     // set by field scanner
	bool trailers;    // parsing trailers
} SpHttp;

typedef enum {
	SP_JSON_NONE       = -1,
	SP_JSON_OBJECT     = '{',
	SP_JSON_OBJECT_END = '}',
	SP_JSON_ARRAY      = '[',
	SP_JSON_ARRAY_END  = ']',
	SP_JSON_STRING     = '"',
	SP_JSON_NUMBER     = '#',
	SP_JSON_TRUE       = 't',
	SP_JSON_FALSE      = 'f',
	SP_JSON_NULL       = 'n'
} SpJsonType;

typedef struct {
	SpUtf8 utf8;        // stores the unescaped string
	double number;      // parsed number value
	SpJsonType type;    // type of the captured value
	unsigned cs;        // current scanner state
	size_t off;         // internal offset mark
	size_t mark;        // mark position for scanning doubles
	uint16_t depth;     // stack entry size
	uint8_t stack[64];  // object/array bit stack
} SpJson;

extern void
sp_http_init_request (SpHttp *p);

extern void
sp_http_init_response (SpHttp *p);

extern void
sp_http_reset (SpHttp *p);

extern int64_t
sp_http_next (SpHttp *p, const void *restrict buf, size_t len);

extern bool
sp_http_is_done (const SpHttp *p);

extern int
sp_json_init (SpJson *p);

extern ssize_t
sp_json_next (SpJson *p, const void *restrict buf, size_t len, bool eof);

extern bool
sp_json_is_done (const SpJson *p);
]]


local HTTP = {}
HTTP.__index = HTTP


function HTTP:__new()
	return ffi.new(self)
end


function HTTP:__tostring()
	local name
	if self.response then
		name = "Response"
	else
		name = "Request"
	end
	return string.format("siphon.http.%s: %p", name, self)
end


function HTTP:init_request()
	lib.sp_http_init_request(self)
	return self
end


function HTTP:init_response()
	lib.sp_http_init_response(self)
	return self
end


function HTTP:reset()
	 lib.sp_http_reset(self)
	 return self
end


function HTTP:next(buf, len)
	return lib.sp_http_next(self, buf, len)
end

function HTTP:is_done()
	return lib.sp_http_is_done(self)
end


function HTTP:has_value()
	return
		self.type == C.SP_HTTP_REQUEST or
		self.type == C.SP_HTTP_RESPONSE or
		self.type == C.SP_HTTP_FIELD or
		self.type == C.SP_HTTP_BODY_START or
		self.type == C.SP_HTTP_BODY_CHUNK
end


function HTTP:value(buf)
	if self.type == C.SP_HTTP_REQUEST then
		return
			ffi.string(
				buf + self.as.request.method_off, self.as.request.method_len),
			ffi.string(buf + self.as.request.uri_off, self.as.request.uri_len),
			self.as.request.version
	elseif self.type == C.SP_HTTP_RESPONSE then
		return
			self.as.response.status,
			ffi.string(
				buf + self.as.response.reason_off, self.as.response.reason_len),
			self.as.request.version
	elseif self.type == C.SP_HTTP_FIELD then
		return
			ffi.string(buf + self.as.field.name_off, self.as.field.name_len),
			ffi.string(buf + self.as.field.value_off, self.as.field.value_len)
	elseif self.type == C.SP_HTTP_BODY_START then
		return
			self.as.body_start.chunked,
			self.as.body_start.content_length
	elseif self.type == C.SP_HTTP_BODY_CHUNK then
		return
			true,
			self.as.body_chunk.length
	elseif self.type == C.SP_HTTP_BODY_END then
		return
			false,
			0
	end
end


local http_alloc = ffi.metatype("SpHttp", HTTP)


return {
	http = {
		Request = function()
			return http_alloc():init_request()
		end,

		Response = function()
			return http_alloc():init_response()
		end
	}
}

