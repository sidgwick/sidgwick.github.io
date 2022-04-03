#!/bin/bash

gem install bundler:2.3.10
bundler --version
gem install rexml -v 3.2.5
bundle install

jekyll build
# jekyll serve -H 0.0.0.0


