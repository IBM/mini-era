FROM centos:7

WORKDIR /home

# Add repo for cmake3
RUN yum install -y epel-release && \
    rpm -ivh http://repo.okay.com.mx/centos/7/x86_64/release/okay-release-1-3.el7.noarch.rpm?
# install python, perl,  CAD tools and software toolchain dependencies, miscellaneous
RUN yum install -y python python-pip python-devel python3 python3-pip python3-devel python3-tkinter \
    perl perl-Env perl-YAML perl-XML-Simple perl-ExtUtils-MakeMaker \
    perl-Thread-Queue perl-Capture-Tiny \
    xterm csh ksh zsh tcl glibc-devel glibc-devel.i686 \
    glibc-static glibc-static.i686 mesa-libGL.i686 mesa-libGLU.i686 mesa-libGL \
    mesa-libGLU mesa-dri-drivers mesa-dri-drivers.i686 readline-devel \
    readline-devel.i686 libXp libXp.i686 openmotif ncurses gdbm-devel \
    gdbm-devel.i686 libSM libSM.i686 libXcursor libXcursor.i686 libXft \
    libXft.i686 libXrandr libXrandr.i686 libXScrnSaver libXScrnSaver.i686 \
    libmpc-devel libmpc-devel.i686 nspr nspr.i686 nspr-devel nspr-devel.i686 \
    tk tk-devel Xvfb dtc bison flex socat minicom tmux \
    git wget bzip2 which unzip file patch bc \
    texinfo texinfo-tex help2man emacs vim xorg-x11-apps java \
    libopencl-devel boost-devel ninja-build gcc-c++ libmpc-devel mpfr-devel \
    gmp-devel pygame autoconf automake openssl-devel libtool --nogpgcheck

RUN ln -s /lib64/libtiff.so.5 /lib64/libtiff.so.3 && \
    ln -s /usr/lib64/libmpc.so.3 /usr/lib64/libmpc.so.2 && \
    #ln -s /usr/bin/cmake3 /usr/bin/cmake && \
    # enable X11 forwardingOA
    sed -i 's/#   ForwardX11 no/ForwardX11 yes/g' /etc/ssh/ssh_config

RUN wget https://cmake.org/files/v3.18/cmake-3.18.0-rc1.tar.gz && tar xvzf cmake-3.18.0-rc1.tar.gz && cd cmake-3.18.0-rc1 && ./bootstrap && make && make install 

# Install Z3 Library needed by LLVM
RUN wget http://mirrors.concertpass.com/gcc/releases/gcc-5.1.0/gcc-5.1.0.tar.gz && \
    tar xvf gcc-5.1.0.tar.gz && cd gcc-5.1.0 &&  \
    ./configure --disable-multilib --enable-languages=c,c++ && make -j4 && \
    make install && cd .. && git clone https://github.com/Z3Prover/z3.git && cd z3 && \
    git checkout tags/z3-4.7.1 && python scripts/mk_make.py && cd build \
    && make -j4 && make install 

#Need jpeg for pillow which is needed for torchvision
RUN yum install -y libjpeg-devel

# create non-root user
RUN useradd -ms /bin/bash espuser
# switch to non-root user
USER espuser
WORKDIR /home/espuser

# Install Anaconda
RUN  wget https://repo.anaconda.com/archive/Anaconda3-2019.07-Linux-x86_64.sh && \
      (echo y; echo yes; echo /home/espuser/anaconda3 ; echo yes) | bash Anaconda3-2019.07-Linux-x86_64.sh

RUN echo "export LD_LIBRARY_PATH=/usr/local/lib64/:/usr/local/lib/" >> /home/espuser/.bashrc
ENV LD_LIBRARY_PATH="/usr/local/lib64/:/usr/local/lib/"
ENV PATH /home/espuser/anaconda3/bin:$PATH

# clone ESP repository and install software toolchains
RUN git clone https://github.com/sld-columbia/esp.git && cd esp && \
    git submodule update --init --recursive \
    accelerators/stratus_hls/common/inc \
    accelerators/chisel/hw rtl/caches/esp-caches rtl/cores/ariane/ariane \
    soft/ariane/riscv-pk soft/ariane/riscv-tests \
    accelerators/third-party/NV_NVDLA/ip \
    accelerators/third-party/NV_NVDLA/sw && \
    cd /home/espuser/esp/accelerators/third-party/NV_NVDLA && \
    rm -rf ip/verif sw/prebuilt sw/regression && \
    cd /home/espuser/esp && \
    (echo y; echo /home/espuser/riscv; echo 4; echo n; echo n; echo n) | bash utils/toolchain/build_riscv_toolchain.sh && rm -rf /tmp/_riscv_build 
#(echo y; echo /home/espuser/riscv32imc; echo 4; echo n) | bash utils/toolchain/build_riscv32imc_toolchain.sh && rm -rf /tmp/_riscv32imc_build

# HPVM Mini-era
#COPY --chown=espuser:espuser hpvm-miniera.tar.gz . 
#RUN tar xvf hpvm-miniera.tar.gz && rm hpvm-miniera.tar.gz && \
  #    cd HPVM-EPOCHS-MINIERA/hpvm-epochs/hpvm && \
  #  printf 'y\n4\nX86;RISCV\n' | bash install.sh
#RUN cd /home/espuser/HPVM-EPOCHS-MINIERA/approxhpvm-nvdla/ && \
  #    mkdir build && cd build && cmake -G "Ninja" ../llvm \ 
  #   -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DCMAKE_BUILD_TYPE=MinSizeRel \
  #    -DLLVM_TARGETS_TO_BUILD="X86" && ninja -j4 
RUN git clone -b hpvm-release-epochs0 --single-branch https://gitlab.engr.illinois.edu/llvm/hpvm-release.git && cd hpvm-release/hpvm && conda init bash && . ~/.bashrc && conda env create -n hpvm -f ./env.yaml && conda activate hpvm && conda install pip libgcc && pip install Pmw torch lightnet && conda update numpy && sed -i 's/--recursive\"])/--recursive\"],cwd=\"\/home\/espuser\/hpvm-release\")/g' scripts/hpvm_installer.py && ./install.sh -t "X86;RISCV"
RUN git clone -b hpvm-integration https://github.com/IBM/scheduler-library.git && \
      cd scheduler-library && conda init bash && . ~/.bashrc && conda activate hpvm && \
      #ln -sf /home/espuser/scheduler-library/sched_library/build_riscv_nvdla.config /home/espuser/scheduler-library/sched_library/hardware_config && \
      sed -i 's/HPVM_DIR=/HPVM_DIR=\/home\/espuser\/hpvm-release\/hpvm/g' setup_paths.sh && \
      sed -i 's/RISCV_BIN_DIR=/RISCV_BIN_DIR=\/home\/espuser\/riscv\/bin/g' setup_paths.sh &&\
      sed -i 's/ESP_ROOT=/ESP_ROOT=\/home\/espuser\/esp/g' setup_paths.sh &&\
      . ./setup_paths.sh && ln -sf /home/espuser/scheduler-library/hw_config_files/build_riscv_nvdla.config /home/espuser/scheduler-library/hardware_config && make clean && make hpvm-epochs && cd examples/mini-era && \
      make CONFIG_VERBOSE=y hpvm-epochs
#      bash -c "source setup_paths.sh; make clean; make hpvm-epochs;"
# && make clean && make hpvm-epochs 
#RUN git clone -b hpvm https://github.com/IBM/mini-era.git && cd mini-era && \
  #    source ./setup_paths.sh && make epochs && cd && \
  #    git clone https://github.com/AkashIwnK/nvdla_inputs_repo.git  

USER root

# enable X11 forwarding
RUN sed -i 's/#   ForwardX11 no/ForwardX11 yes/g' /etc/ssh/ssh_config
# copy over environment setup script (without CAD tools)
COPY small/esp_env.sh /home/espuser
# copy over environment setup script (with CAD tools)
COPY small/esp_env_cad.sh /home/espuser
# copy over bashrc script
COPY small/bashrc /root/.bashrc
# copy over tmux.conf script
COPY small/tmux.conf /root/.tmux.conf
# copy over emacs script
COPY small/emacs /root/.emacs
