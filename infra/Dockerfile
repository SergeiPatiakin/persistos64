FROM debian:bookworm@sha256:00cd074b40c4d99ff0c24540bdde0533ca3791edcdac0de36d6b9fb3260d89e2
RUN apt update
COPY debian-packages.lock .
# RUN apt install -y clang make curl xorriso gdb vim bsdmainutils
RUN xargs -a debian-packages.lock apt install -y
