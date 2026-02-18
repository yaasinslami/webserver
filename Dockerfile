FROM ubuntu:latest

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    git \
    valgrind \
    gdb \
    curl \
    wget \
    net-tools \
    iputils-ping \
    tcpdump \
    vim \
    zsh \
    python3 \
    python3-pip \
    php-cgi \
    && rm -rf /var/lib/apt/lists/*

RUN sh -c "$(curl -fsSL https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh)" "" --unattended

SHELL ["/bin/zsh", "-c"]

RUN git clone https://github.com/zsh-users/zsh-autosuggestions ${ZSH_CUSTOM:-~/.oh-my-zsh/custom}/plugins/zsh-autosuggestions
RUN sed -i 's/plugins=(git)/plugins=(git zsh-autosuggestions)/' ~/.zshrc

WORKDIR /app

CMD ["tail", "-f", "/dev/null"]