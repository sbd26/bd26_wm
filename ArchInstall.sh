#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
RESET='\033[0m'


echo -e "${GREEN}Installing xorg and Basic Util ${RESET}"
sudo pacman -S xorg xorg-xinit gcc make

PROG=""

if which paru >/dev/null 2>&1; then
    echo "[*] Found Paru"
    PROG="paru"
elif which yay >/dev/null 2>&1; then
  echo "[*] Found Yay"
  PROG="yay"
else
  echo "${RED}[!] Neither yay nor paru is installed.${RESET}"
  read -p "Enter 1 to install paru, or anything else to install yay: " choice

  if [[ $choice == "1" ]]; then
    echo "[*] Installing paru"
    git clone https://aur.archlinux.org/paru-bin.git
    cd paru-bin
    makepkg -si
    cd ..
    PROG="paru"
  else
    echo "[*] Installing Yay"
    git clone https://aur.archlinux.org/yay-bin.git
    cd yay-bin
    makepkg -si
    cd ..
    PROG="yay"
  fi
fi

echo -e "${GREEN}Insatlling picom and polybar${RESET}"
$PROG -S picom-pijulius-git polybar rofi 
cp -r picom ~/.config
sudo make install
sudo cp -f bd26.desktop /usr/share/applications


echo -e "${YELLOW} Installing Nerd Font for Polybar"

directory="~/.fonts"

if [ -d "$directory" ]; then
  echo "Directory exists."
  cp font/*.ttf ~/.fonts/
else
  mkdir ~/.fonts
  cp font/*.ttf ~/.fonts/
fi
