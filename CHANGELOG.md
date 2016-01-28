# Changelog

## 0.2.4-alpha

* allow `;` a cache control directive delimiter (improper, but used in the wild)
* fix error iterator overrun

## 0.2.3

* add functions to query json and msgpack parser state
* add configurable limits to the http parser
* move quote termination checks into the utf8 json function
* remove support for NULL entries in map
* fix path walking when hitting an empty directory
* fix entry identity in hash ring

## 0.2.2

* change behavior when using `sp_map_reserve`
* fix empty msgpack array/map end state

## 0.2.1

* add inlined bsearch and search so that comparison functions can be typed
* fix hash ring error when finding on an empty ring
* add xxHash 64-bit function
* track unfreed memory in debugging allocator
* integrate CircleCI

## 0.2.0 - 2015-12-04

* first tagged release
