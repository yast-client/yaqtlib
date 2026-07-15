# yaqtlib

yaqtlib is a yet another Qt TDLib Library. It is designed for creating Telegram clients for Mer-based systems.

## Credits

All Fernschreiber developers, including but not limited to:

- Sebastian J. Wolf [sebastian@ygriega.de](mailto:sebastian@ygriega.de)
- [Slava Monich](https://github.com/monich)
- [jgibbon](https://github.com/jgibbon)
- [Christian Stemmle](https://github.com/chstem)
- [santhoshmanikandan](https://github.com/santhoshmanikandan)
- [Peter G.](https://github.com/nephros)
- [Johannes Bachmann](https://github.com/dscheinah)
- [Mikhail Barashkov](https://github.com/mbarashkov)

This project uses

- The Telegram Database Library (TDLib) - available on [GitHub.com](https://github.com/tdlib/td). Thanks for making it available under the conditions of the Boost Software License 1.0! Details about the license of TDLib in [its license file](https://github.com/tdlib/td/blob/master/LICENSE_1_0.txt).
- Animated sticker parsing and animation by [rlottie](https://github.com/Samsung/rlottie), copyright 2020 Samsung Electronics Co., Ltd. and [other contributors](https://github.com/Samsung/rlottie/blob/master/AUTHORS), Code licensed under the [MIT License](https://github.com/Samsung/rlottie/blob/master/licenses/COPYING.MIT), some rlottie components [licensed under other licenses](https://github.com/Samsung/rlottie/blob/master/COPYING).
- Reverse geocoding for location attachments by [OpenStreetMap Nominatim](https://wiki.openstreetmap.org/wiki/Nominatim).
- Calls work through [tgcalls](https://github.com/TelegramMessenger/tgcalls) - the Telegram Calls Library. Thanks for making it available under the [GNU LGPL V3 license](https://github.com/TelegramMessenger/tgcalls/tree/master/LICENSE)!
- [tg_owt](https://github.com/desktop-app/tg_owt) (WebRTC). Thanks for making it available under the [BSD 3-Clause license](https://github.com/desktop-app/tg_owt/blob/master/LICENSE)!

## Usage

First, you would need to implement `PlatformApp`. See `platformapp.h` for details, or `platformapp.cpp` in YAST source for an example.

The installation of TDLib should be handled by the application. **The version of TDLib currently supported by yaqtlib is 1.8.66.**

See YAST for a complete example of including yaqtlib in your project.

**Important:** yaqtlib's API is not stable, and it is not versioned. YAST generally adapts to the latest API changes, since that is the main project relying on this library. This is unlikely to change in the future, since no code is ideal and there's always room for improvement.

## TODO

- Move LottieItem to a separate project