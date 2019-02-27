//
// Created by root on 2/11/19.
// This is an implementation of LRU cache to store cached request from proxy

#ifndef CACHING_PROXY_CACHE_H
#define CACHING_PROXY_CACHE_H

#include <iostream>
#include <unordered_map>
#include <string>
#include <list>
#include "parse.h"
#include <glog/logging.h>

using namespace std;

/*
 * This cache contains a hash table and a linked list,
 * hash table records key-value pair as well as position in linked list,
 * linked list simulate LRU process which head is most recent used and tail is one could be dropped.
 */
class cache{
    typedef string Key;
    typedef response Value;
    typedef pair<Value, list<Key>::iterator> list_pair;
    typedef pair<Key, list_pair > map_pair;
    typedef unordered_map<Key, list_pair> hash_map;
private:
    hash_map cache_map;
    list<Key> cache_list;
    size_t max_size;

public:
    cache():cache_map(hash_map()), cache_list(list<Key>()), max_size(5){

    }

    explicit cache(size_t max_size_):cache_map(hash_map()), cache_list(list<Key>()), max_size(max_size_){

    }

    /*
     * find if there is such a key in cache.
     * Return: true if exists, otherwise false.
     */
    bool find(const Key& key){
        return cache_map.find(key) != cache_map.end();
    }

    size_t get_max_size(){
        return max_size;
    }

    size_t get_size(){
        return cache_list.size();
    }

    /*
     * insert a key-value pair into cache.
     * Logic: if there is a same key erase it, if cache is full try to drop tail.
     * Always insert at head of linked list.
     */
    void insert(const Key& key, const Value& value){
        if(find(key)){
            erase(key, "");
        }

        if(cache_list.size() == max_size){
            Key last_key = *cache_list.rbegin();
            cache_list.erase(--cache_list.end());
            cache_map.erase(last_key);
        }
        cache_list.push_front(key);
        cache_map.insert(map_pair(key, list_pair(value, cache_list.begin())));
    }

    /*
     * get a value pair response to specific key, MUST run get first.
     * Logic: get value reponsed to key and move it to head.
     */
    Value get(const Key& key){
        Value value = cache_map.find(key)->second.first;
        if(cache_map.find(key)->second.second != cache_list.begin()){
            erase(key, "");
            cache_list.push_front(key);
            cache_map.insert(map_pair(key, list_pair(value, cache_list.begin())));
        }
        return value;
    }

    /*
     * erase a record in cache.
     * NOTE: if it is called by proxy not cache itself, it will write a note into log.
     */
    void erase(const Key& key, const string& url){
        if(!url.empty()) LOG(INFO) << "(no-id): NOTE erase " << url << " from cache"<< endl;
        cache_list.erase(cache_map.find(key)->second.second);
        cache_map.erase(key);
    }
};

#endif //CACHING_PROXY_CACHE_H
