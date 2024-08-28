FROM archlinux:latest as build

RUN pacman -Syu --noconfirm curl wget ninja pkgconf clang cmake make git \
        patch ca-certificates gd opus zlib ffmpeg flac nodejs npm lmdb
COPY . /opt/mix_build
RUN cd /opt/mix_build && make release && \
    cd /opt/mix_build && make webui && \
    mkdir -p /opt/mix/webui/public && \
    cp -a /opt/mix_build/build /opt/mix/ && \
    cp -a /opt/mix_build/webui/dist /opt/mix/webui/ 

# --- Final image ---
FROM archlinux:latest 
RUN pacman -Syu --noconfirm ca-certificates gd opus zlib ffmpeg flac lmdb && \
    yes | pacman -Scc

COPY --from=build /opt/mix /opt/mix

VOLUME /opt/mix/webui/public/avatars
VOLUME /opt/mix/webui/public/downloads
VOLUME /opt/mix/webui/database

USER root
ENV USER=root

ADD entrypoint.sh /entrypoint.sh

EXPOSE 9999/tcp

ENTRYPOINT ["/entrypoint.sh"]
CMD ["slmix"]
