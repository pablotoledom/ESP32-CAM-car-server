#!/bin/bash
rm web_index.h
gzip -k frontend.html
xxd  -i frontend.html.gz ./web_index.h
rm *.gz