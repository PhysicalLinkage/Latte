FROM "ubuntu:18.04"

ARG D_USER=""
ARG D_UID=""
ARG D_GID=""
ARG D_UMASK=""

USER root

RUN apt-get update && apt-get install -yq --no-install-recommends \
        sudo \
        locales \
        vim \
        git \
 && apt-get clean \
 && rm -rf /var/lib/apt/lists/*


RUN apt-get update && apt-get install -yq --no-install-recommends \
        build-essential \
        libssl-dev \
 && apt-get clean \
 && rm -rf /var/lib/apt/lists/*

RUN echo "en_US.UTF-8 UTF-8" > /etc/locale.gen \
 && locale-gen

RUN groupadd -o -g $D_GID $D_USER \
 && useradd -m -o -u $D_UID -g $D_GID $D_USER \
 && adduser $D_USER sudo

USER $D_USER

RUN echo umask $D_UMASK >> ~/.bashrc \
 && mkdir /home/${D_USER}/work

WORKDIR /home/${D_USER}/work

CMD ["/bin/bash"]
