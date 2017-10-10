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
        libssl-dev \
        libfontconfig1-dev \
        libfreetype6-dev \
        libx11-dev \
        libxext-dev \
        libxfixes-dev \
        libxi-dev \
        libxrender-dev \
        libxcb1-dev \
        libx11-xcb-dev \
        libxcb-glx0-dev \
        libgl1-mesa-dev \
        libglu1-mesa-dev \
        mdm \
        libdbus-1-dev \
        libxcomposite-dev \
        libxcursor-dev \
        libxrandr-dev \
        libxtst-dev \
        libegl1-mesa-dev \
        bison \
        flex \
        gperf \
        ninja-build \
        git

# TODO: remove when >= 5.8 is available
RUN	cd && \
    wget http://download.qt.io/official_releases/qt/5.9/5.9.1/single/qt-everywhere-opensource-src-5.9.1.tar.xz && \
    tar xf qt-everywhere-opensource-src-5.9.1.tar.xz && \
    rm qt-everywhere-opensource-src-5.9.1.tar.xz && \
    cd ~/qt-everywhere-opensource-src-5.9.1 && \
    update-alternatives --set c++ /usr/bin/g++ && \
    update-alternatives --set cc /usr/bin/gcc && \
    ./configure -opensource -confirm-license -release -c++std c++1z -make libs -ssl && \
    make -j`ncpus` && \
    make install && \
    update-alternatives --set c++ /usr/bin/clang++-4.0 && \
    update-alternatives --set cc /usr/bin/clang-4.0 && \
    cd && rm -rf qt-everywhere-opensource-src-5.9.1

RUN cd && \
    wget https://cmake.org/files/v3.8/cmake-3.8.2.tar.gz && \
    tar xf cmake-3.8.2.tar.gz && \
    cd cmake-3.8.2 && \
    ./configure --prefix=/usr && \
    make -j`ncpus` && \
    make install && \
    cd && rm -rf cmake-3.8.2

# we don't want to use cache for the following layers, but docker cannot selectively disable it
ADD .cache-bust /root

RUN hg clone -u latest-release https://krojew@bitbucket.org/krojew/evernus

WORKDIR evernus
RUN cmake \
    -DEVERNUS_CLIENT_ID=$EVERNUS_CLIENT_ID -DEVERNUS_CLIENT_SECRET=$EVERNUS_CLIENT_SECRET \
    -DEVERNUS_DROPBOX_APP_KEY=$EVERNUS_DROPBOX_APP_KEY -DEVERNUS_DROPBOX_APP_SECRET=$EVERNUS_DROPBOX_APP_SECRET \
    -DEVERNUS_CREATE_DUMPS=OFF \
    -DCMAKE_PREFIX_PATH=/usr/local/Qt-5.9.1/lib/cmake \
    -G 'Unix Makefiles' && \
    make -j8 && \
    make install && \
    cd .. && rm -rf evernus

ENV QT_PLUGIN_PATH=/usr/local/Qt-5.9.1/plugins
ENV QML2_IMPORT_PATH=/usr/local/Qt-5.9.1/qml

ENV LD_LIBRARY_PATH=/usr/local/Qt-5.9.1/lib

ENTRYPOINT /usr/local/bin/evernus
