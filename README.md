# kpmpm - Kate plugin for the Pandoc markdown preview machine

_This is a combination of the [Kate preview addon](https://invent.kde.org/utilities/kate/) and the [KMarkdown Webview](https://invent.kde.org/utilities/kmarkdownwebview) that is customized to use [pmpm - pandoc markdown preview machine](https://github.com/sweichwald/pmpm)_

---

## Building

Build and install kate addon, e.g.
```
$ cd kateaddon/
$ mkdir build && cd build && cmake ../ && make
$ sudo make install
```

Build and install viewer (i.e. the markdown kpart), e.g.
```
$ cd viewer/
$ mkdir build && cd build && cmake ../ && make
$ sudo make install
```

*NOTE*: At the moment this overwrites the original kate preview addon and the original kmarkdownwebview installation (if present). This should be improved by renaming things.
