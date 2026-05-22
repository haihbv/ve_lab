mkdir ~/ve_lab/arm_gcc
cd ~/ve_lab/arm_gcc

sudo apt-get install wget
wget https://developer.arm.com/-/media/Files/downloads/gnu/15.2.rel1/binrel/arm-gnu-toolchain-15.2.rel1-aarch64-arm-none-eabi.tar.xz
tar -xf arm-gnu-toolchain-15.2.rel1-aarch64-arm-none-eabi.tar.xz

echo 'export PATH="$PATH:/home/pi/ve_lab/arm_gcc/arm-gnu-toolchain-15.2.rel1-aarch64-arm-none-eabi/bin"' >> ~/.bashrc
source ~/.bashrc

echo $PATH 
arm-none-eabi-gcc --version

sudo apt install -y stlink-tools
st-flash --version

st-info --probe

st-flash --format=ihex write build/template_make.hex 

