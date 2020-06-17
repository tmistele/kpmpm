# kpmpm - Kate plugin for the Pandoc markdown preview machine

_This is a combination of the [Kate preview addon](https://invent.kde.org/utilities/kate/) and the [KMarkdown Webview](https://invent.kde.org/utilities/kmarkdownwebview) that is customized to use [pmpm - pandoc markdown preview machine](https://github.com/sweichwald/pmpm)_

---

## Building and installing

*NOTE*: At the moment this overwrites the original kate preview addon and the original kmarkdownwebview installation (if present). This should be improved by renaming things.

### Install to home directory

Build and install kate addon, e.g.
```
$ cd kateaddon/
$ mkdir build && cd build
$ cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/.local
$ make -j5
$ make install
```

Build and install viewer (i.e. the markdown kpart), e.g.
```
$ cd viewer/
$ mkdir build && cd build
$ cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/.local
$ make -j5
$ make install
```

You may need to set the `QT_PLUGIN_PATH` variable before you run `kate`, e.g.:
```
$ export QT_PLUGIN_PATH=$HOME/.local/lib64/plugins
$ kate
```

### Install to system

To install for the whole system (not just a single user), you can leave out the `-DCMAKE_INSTALL_PREFIX` switch and use `sudo make install` instead of `make install`.
This should work without setting `QT_PLUGIN_PATH`.
