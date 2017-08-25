FROM krojew/ubuntu-clang

ARG EVERNUS_CLIENT_ID
ARG EVERNUS_CLIENT_SECRET

ARG EVERNUS_DROPBOX_APP_KEY
ARG EVERNUS_DROPBOX_APP_SECRET

RUN apt-get update
RUN apt-get install -y qtbase5-dev qttools5-dev-tools qtbase5-private-dev qtdeclarative5-dev libqt5xmlpatterns5-dev qtpositioning5-dev qtwebengine5-dev qtmultimedia5-dev mercurial wget libboost-all-dev make

RUN cd && \
    wget https://cmake.org/files/v3.8/cmake-3.8.2.tar.gz && \
    tar xf cmake-3.8.2.tar.gz && \
    cd cmake-3.8.2 && \
    ./configure --prefix=/usr && \
    make -j`ncpus` && \
    make install && \
    cd && rm -rf cmake-3.8.2

RUN hg clone -u latest-release https://krojew@bitbucket.org/krojew/evernus

WORKDIR evernus
RUN cmake \
    -DEVERNUS_CLIENT_ID=$EVERNUS_CLIENT_ID -DEVERNUS_CLIENT_SECRET=$EVERNUS_CLIENT_SECRET \
    -DEVERNUS_DROPBOX_APP_KEY=$EVERNUS_DROPBOX_APP_KEY -DEVERNUS_DROPBOX_APP_SECRET=$EVERNUS_DROPBOX_APP_SECRET \
    -DEVERNUS_CREATE_DUMPS=OFF \
    -G 'Unix Makefiles' && \
    make -j8 && \
    make install && \
    cd .. && rm -rf evernus

ENTRYPOINT /usr/local/bin/evernus
