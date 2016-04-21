require 'pp'
require 'msgpack'

def count(val)
  case val
  when Array
    val.reduce(2) { |acc, v| acc + count(v) }
  when Hash
    val.reduce(2) { |acc, v| acc + count(v[0]) + count(v[1]) }
  else
    1
  end
end

def assertions(val, i=0)
  case val
  when Array
    puts "\tmu_assert_int_eq (msg.fields[#{i}].type, SP_MSGPACK_ARRAY);"
    puts "\tmu_assert_uint_eq (msg.fields[#{i}].tag.count, #{val.length});"
    i += 1
    val.each do |v|
      i = assertions(v, i)
    end
    puts "\tmu_assert_int_eq (msg.fields[#{i}].type, SP_MSGPACK_ARRAY_END);"
    i += 1
  when Hash
    puts "\tmu_assert_int_eq (msg.fields[#{i}].type, SP_MSGPACK_MAP);"
    puts "\tmu_assert_uint_eq (msg.fields[#{i}].tag.count, #{val.length});"
    i += 1
    val.each do |k,v|
      i = assertions(k, i)
      i = assertions(v, i)
    end
    puts "\tmu_assert_int_eq (msg.fields[#{i}].type, SP_MSGPACK_MAP_END);"
    i += 1
  when NilClass
    puts "\tmu_assert_int_eq (msg.fields[#{i}].type, SP_MSGPACK_NIL);"
    i += 1
  when TrueClass
    puts "\tmu_assert_int_eq (msg.fields[#{i}].type, SP_MSGPACK_TRUE);"
    i += 1
  when FalseClass
    puts "\tmu_assert_int_eq (msg.fields[#{i}].type, SP_MSGPACK_FALSE);"
    i += 1
  when Fixnum
    if val < 0 then
      puts "\tmu_assert_int_eq (msg.fields[#{i}].type, SP_MSGPACK_SIGNED);"
      puts "\tmu_assert_int_eq (msg.fields[#{i}].tag.i64, #{val});"
    else
      puts "\tmu_assert_int_eq (msg.fields[#{i}].type, SP_MSGPACK_UNSIGNED);"
      puts "\tmu_assert_uint_eq (msg.fields[#{i}].tag.u64, #{val});"
    end
    i += 1
  when Float
    puts "\tmu_assert_int_eq (msg.fields[#{i}].type, SP_MSGPACK_DOUBLE);"
    i += 1
  when String
    puts "\tmu_assert_int_eq (msg.fields[#{i}].type, SP_MSGPACK_STRING);"
    puts "\tmu_assert_str_eq (msg.fields[#{i}].str, #{val.inspect});"
    i += 1
  end
  i
end

def dump(name, val)
  puts "static void"
  puts "test_decode_#{name} (ssize_t speed)"
  puts "{"
  puts "\t/*"
  PP.pp(val, "").each_line { |l| print "\t * #{l}" }
  puts "\t */"
  puts "\tstatic const uint8_t input[] = {"
  bytes = val.to_msgpack.each_byte.collect { |b| "0x%02x" % b }
  rows = bytes.each_slice(16).collect { |s| "\t\t#{s.join(",")}" }
  puts rows.join(",\n")
  puts "\t};"
  puts "\tMessage msg;"
  puts "\tmu_fassert (parse (&msg, input, sizeof input, speed));"
  puts "\tmu_fassert_uint_eq (msg.field_count, #{count(val)});"
  assertions(val)
  puts "}\n\n"
end

dump("general", [
  "test",
  "this is a longer string that should not pack to a fixed string",
  true, false,
  12, 123, 1234, 1234567, 123456789000,
  -12, -123, -1234, -1234567, -123456789000,
  {
    'a' => 1.23,
    'b' => true,
    'c' => 'value',
    'd' => [1,2,3],
    'e' => { 'x' => 'y' },
    'f' => {},
    'g' => []
  },
  [{}, []],
  {},
  []
])

dump("nested_begin", {
  "test" => [ "a", "b", "c" ],
  "stuff" => "123",
  "other" => "456"
})

dump("nested_middle", {
  "test" => "123",
  "stuff" => [ "a", "b", "c" ],
  "other" => "456"
})

dump("nested_end", {
  "test" => "123",
  "stuff" => "456",
  "other" => [ "a", "b", "c" ]
})

dump("array_key_begin", {
  ["x", "y", "z"] => [ "a", "b", "c" ],
  "stuff" => "123",
  "other" => "456"
})

dump("array_key_middle", {
  "test" => "123",
  ["x", "y", "z"] => [ "a", "b", "c" ],
  "other" => "456"
})

dump("array_key_end", {
  "test" => "123",
  "stuff" => "456",
  ["x", "y", "z"] => [ "a", "b", "c" ]
})

dump("map_key_begin", {
  {"x" => "y"} => [ "a", "b", "c" ],
  "stuff" => "123",
  "other" => "456"
})

dump("map_key_middle", {
  "test" => "123",
  {"x" => "y"} => [ "a", "b", "c" ],
  "other" => "456"
})

dump("map_key_end", {
  "test" => "123",
  "stuff" => "456",
  {"x" => "y"} => [ "a", "b", "c" ]
})
