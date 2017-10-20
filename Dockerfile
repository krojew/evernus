FROM krojew/ubuntu-clang

ARG EVERNUS_CLIENT_ID
ARG EVERNUS_CLIENT_SECRET

ARG EVERNUS_DROPBOX_APP_KEY
ARG EVERNUS_DROPBOX_APP_SECRET

RUN apt-get update && \
    apt-get install -y \
        mercurial \
        wget \
        libboost-all-dev \
        make \
        mdm \
        qt5-default \
        qtmultimedia5-dev \
        qtdeclarative5-dev \
        qtwebengine5-dev \
        qttools5-dev-tools \
        qtbase5-private-dev \
        libqt5xmlpatterns5-dev \
        cmake

# we don't want to use cache for the following layers, but docker cannot selectively disable it
ADD .cache-bust /root

RUN hg clone -u latest-release https://krojew@bitbucket.org/krojew/evernus

WORKDIR evernus
RUN cmake \
    -DEVERNUS_CLIENT_ID=$EVERNUS_CLIENT_ID -DEVERNUS_CLIENT_SECRET=$EVERNUS_CLIENT_SECRET \
    -DEVERNUS_DROPBOX_APP_KEY=$EVERNUS_DROPBOX_APP_KEY -DEVERNUS_DROPBOX_APP_SECRET=$EVERNUS_DROPBOX_APP_SECRET \
    -DEVERNUS_CREATE_DUMPS=OFF \
    -G 'Unix Makefiles' && \
    make -j`ncpus` && \
    make install && \
    cd .. && rm -rf evernus

ENTRYPOINT /usr/local/bin/evernus
