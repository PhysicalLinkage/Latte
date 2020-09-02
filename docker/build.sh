#!/bin/bash

d_user=${USER}
d_uid=$(id -u)
d_gid=$(id -g)
d_umask=$(umask)
d_mount_dir=$(pwd)

d_name_version=(${1//:/ })
if [ ${#d_name_version[@]} -eq 2 ]; then
    d_tag_name=${d_name_version[0]}
    d_tag_version=${d_name_version[1]}
elif [ ${#d_name_version[@]} -eq 1 ]; then
    d_tag_name=${d_name_version[0]}
    d_tag_version="latest"
else
    echo "Usage : $ bash build.sh [tag_name]"
    exit 1
fi

d_image_name=${d_user}/${d_tag_name}:${d_tag_version}
d_container_name=${d_tag_name}

docker build \
    --build-arg D_USER=${d_user} \
    --build-arg D_UID=${d_uid} \
    --build-arg D_GID=${d_gid} \
    --build-arg D_UMASK=${d_umask} \
    --tag ${d_user}/${d_tag_name}:${d_tag_version} \
    docker/
