#!/bin/bash

IMAGE_NAME="picopi-build"

build() {
    echo "Building docker image: '$IMAGE_NAME'"
    pushd "$(dirname ${BASH_SOURCE[0]})"
    docker build -t "$IMAGE_NAME" "$@" .

    # Check if the build was successful
    if [ $? -ne 0 ]; then
        echo "Docker image build failed. Exiting."
        exit 1
    fi
    popd
}

if ! docker image inspect "$IMAGE_NAME" &> /dev/null; then
    build
fi

if [ "$1" == "--rebuild" ]; then
  build
  shift
fi

if [ "$1" == "--cleanbuild" ]; then
  build --no-cache
  shift
fi

# Get the current user's UID and GID
USER_ID=$(id -u)
GROUP_ID=$(id -g)

if [ "$#" -eq 0 ]
then
    docker run --rm -it \
        -v "$(pwd)":"$(pwd)" -w"$(pwd)" \
        -v /dev/bus/usb:/dev/bus/usb    \
        --privileged                    \
        --user "$USER_ID:$GROUP_ID"     \
        $IMAGE_NAME bash
else
    docker run --rm -it \
        -v "$(pwd)":"$(pwd)" -w"$(pwd)" \
        -v /dev/bus/usb:/dev/bus/usb    \
        --privileged                    \
        --user "$USER_ID:$GROUP_ID"     \
        $IMAGE_NAME bash -c "$*"
fi
