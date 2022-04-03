FROM ubuntu:latest
RUN apt-get update -q \
  && DEBIAN_FRONTEND=noninteractive apt-get install -qy build-essential ruby ruby-dev graphviz \
  && apt-get clean \
  && rm -rf /var/lib/apt
RUN gem install jekyll bundler
#ENTRYPOINT ["jekyll", "serve"]
ENTRYPOINT ["bin/bash"]
