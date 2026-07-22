# Third-Party Notices

## ItemIntegrationFramework

`src/IIF_API.h` is the public consumer API from
[ItemIntegrationFramework](https://github.com/wushen233/ItemIntegrationFramework).
ItemIntegrationFramework is distributed under the MIT License and is required
at runtime for WRF item card and shared combat modifier integration.

## Xbyak

The `src/xbyak` directory contains Xbyak by MITSUNARI Shigeo. Xbyak is
distributed under the 3-Clause BSD License. Its original copyright and license
text are retained in `src/xbyak/COPYRIGHT`.

## CommonLibF4 and commonlib-shared

This project is built against
[Dear-Modding-FO4/commonlibf4](https://github.com/Dear-Modding-FO4/commonlibf4)
at commit `ca31eeb6c7353555973bc351c6733d6492f2c66e`. CommonLibF4's top-level
code is distributed under the MIT License.

That revision statically links
[Dear-Modding-FO4/commonlib-shared](https://github.com/Dear-Modding-FO4/commonlib-shared)
at commit `f0b1670ee9caac2e349497f6f3c08a69633a8ea7`. `commonlib-shared` is
distributed under GPL-3.0 with a Modding Exception. The exception permits
Modded Code to link with `commonlib-shared` without causing that Modded Code to
be covered by the GPL. Project-authored source in this repository therefore
remains under the repository MIT License.

Copies of the applicable GPL-3.0 text and Modding Exception are retained in
`licenses/commonlib-shared/LICENSE` and `licenses/commonlib-shared/EXCEPTIONS`.

## Build dependencies

The following projects are obtained separately by the build system and are not
vendored in this repository:

- SimpleIni, MIT License.
- JSON for Modern C++, MIT License.

Refer to each dependency's source distribution for its complete license text.
