FROM archlinux:latest as build

RUN pacman -Syu --noconfirm curl wget ninja pkgconf clang cmake make git \
        patch ca-certificates gd opus zlib ffmpeg flac nodejs npm lmdb
COPY . /opt/mix_build
RUN cd /opt/mix_build && make release && \
    cd /opt/mix_build && make webui && \
    mkdir -p /opt/slmix/webui/public && \
    cp -a /opt/mix_build/build /opt/slmix/ && \
    cp -a /opt/mix_build/webui/dist /opt/slmix/webui/ 

# --- Final image ---
FROM archlinux:latest 

COPY --from=build /opt/slmix /opt/slmix

RUN pacman -Syu --noconfirm ca-certificates gd opus zlib ffmpeg flac lmdb \
    nginx sudo && \
    yes | pacman -Scc && \
    useradd slmix -d /opt/slmix -s /bin/bash -u 16371 -U && \
    mkdir -p /opt/slmix/webui/public/avatars && \
    mkdir -p /opt/slmix/webui/public/download && \
    echo "<!doctype html><html lang=\"en\"><title>Download</title><b>Use /download/TOKEN</b></html>" > /opt/slmix/webui/public/download/index.html && \
    mkdir -p /opt/slmix/webui/database && \
    chown -R slmix:slmix /opt/slmix && \
    echo 'slmix ALL=(ALL) NOPASSWD:SETENV: /usr/bin/nginx' >> /etc/sudoers

COPY docker/nginx.conf /etc/nginx/nginx.conf

VOLUME /opt/slmix/webui/public/avatars
VOLUME /opt/slmix/webui/public/download
VOLUME /opt/slmix/webui/database

USER slmix 

ADD entrypoint.sh /entrypoint.sh

EXPOSE 80/tcp

ENTRYPOINT ["/entrypoint.sh"]
CMD ["slmix"]
