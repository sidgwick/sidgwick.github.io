#!/bin/bash

gem install bundler:2.5.6
bundler --version
gem install rexml -v 3.2.5
bundle install

jekyll build --trace
# jekyll serve --trace --incremental -H 0.0.0.0
