FROM ruby:3.1-slim-bullseye as jekyll
RUN apt-get update -q \
    && DEBIAN_FRONTEND=noninteractive apt-get install -qy build-essential ruby ruby-dev graphviz git python3-pip \
    && apt-get clean \
    && rm -rf /var/lib/apt

RUN gem sources --add https://gems.ruby-china.com/ --remove https://rubygems.org/ \
    && gem install jekyll \
    && gem update --system

RUN pip3 install jupyter

WORKDIR /blog
ENTRYPOINT ["/bin/bash", "bootstrap.sh"]
