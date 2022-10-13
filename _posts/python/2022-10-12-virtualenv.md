---
title: "Python 虚拟环境管理 virtualenvwrapper 相关使用"
date: 2022-12-26 10:28:04
tags: python, virtualenv, virtualenvwrapper
---

> https://stackoverflow.com/questions/49470367/install-virtualenv-and-virtualenvwrapper-on-macos
> Virtualenv

To install virtualenv and virtualenvwrapper for repetitive use you need a correctly configured Python (this example uses Python 3.x but process is identical for Python 2.x).

Although you can get python installer from Python website I strongly advice against it. The most convenient and future-proof method to install Python on MacOS is brew.

Main difference between installer from Python website and brew is that installer puts python packages to:

/Library/Frameworks/Python.framework/Versions/3.x
Brew on the other hand installs Python, Pip & Setuptools and puts everything to:

/usr/local/bin/python3.x/site-packages
And though it may not make any difference to you now – it will later on.

Configuration steps
Install brew
Check out brew installation page or simply run this in your terminal:

/usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
Install Python
To install python with brew run:

brew install python3
Now your system needs to know where to look for freshly installed Python packages. Add this line to youre ~/.zshrc (or ~/.bash_profile if you're using bash):

export PATH=/usr/local/share/python:$PATH
Restart your terminal. To make sure you've done everything correctly run which python3 and in return you should receive /usr/local/bin/python.

Install virtualenv & virtualenvwrapper
Now it's time to install virtualenv and virtualenvwrapper to be able to use workon command and switch between virtual environments. This is done using pip:

pip3 install virtualenv virtualenvwrapper
Set up virtualenv variables
Define a default path for your virtual environments. For example you can create a hidden directory inside ~ and called it .virtualenvs with mkdir ~/.virtualenvs. Add virtualenv variables to .zshrc (or .bash_profile).

Final version of your .zshrc (or .bash_profile) should contain this information to work properly with installed packages:

# Setting PATH for Python 3 installed by brew

export PATH=/usr/local/share/python:$PATH

# Configuration for virtualenv

export WORKON_HOME=$HOME/.virtualenvs
export VIRTUALENVWRAPPER_PYTHON=/usr/local/bin/python3
export VIRTUALENVWRAPPER_VIRTUALENV=/usr/local/bin/virtualenv
source /usr/local/bin/virtualenvwrapper.sh
Restart your terminal. You should be able to use mkvirtualenv and workon commands including autocompletion.

Here's a little tip on how to create virtualenv with specific version of Python.

In case you are using MacOS Mojave and you are installing Python3.6 from brew bottle you might have a problem with pip, here's a solution that might help.

With time some of you may want to install multiple Python versions with multiple virtual environments per version. When this moment comes I strongly recommend swithing to pyenv and pyenv-virtualenv .
