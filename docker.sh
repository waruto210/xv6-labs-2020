docker run \
          -d \
          --env PS1="DAC(\#)[\d \T:\w]\\$ " \
          -p 10122:22 \
          --interactive \
          --privileged \
          --rm \
          --tty \
          --volume "/Users/naruto210/docker-share/xv6-lab:/xv6-lab" \
          "registry.hub.docker.com/library/archlinux:latest"
