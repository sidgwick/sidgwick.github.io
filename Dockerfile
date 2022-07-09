FROM ubuntu:latest
RUN apt-get update -q \
  && DEBIAN_FRONTEND=noninteractive apt-get install -qy build-essential ruby ruby-dev graphviz git \
  && apt-get clean \
  && rm -rf /var/lib/apt
# RUN gem sources --add https://gems.ruby-china.com/ --remove https://rubygems.org/ \
#  && gem install jekyll \
 RUN gem install jekyll \
 && gem update --system
WORKDIR /blog
ENTRYPOINT ["/bin/bash", "bootstrap.sh"]
# ENTRYPOINT ["bin/bash"]
