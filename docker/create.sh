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
    echo "Usage : $ bash create.sh [tag_name]"
    exit 1
fi

d_image_name=${d_user}/${d_tag_name}:${d_tag_version}
d_container_name=${d_tag_name}

docker create \
    -it \
    --ipc=host \
    --net=host \
    --gpus all \
    -e D_USER=${d_user} \
    -e D_UID=${d_uid} \
    -e D_GID=${d_gid} \
    -e D_UMASK=${d_umask} \
    -v "${d_mount_dir}":/home/${d_user}/work \
    --name=${d_container_name} \
    ${d_image_name}
