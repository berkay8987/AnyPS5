# libSceJsc

## add Functional

- [WTF::JSONImpl::Value::escapeString](../../../core/libs/prx/libSceJsc/src/WtfJsonEscape.cpp) (libSceJsc) - not compared with a console module; the console WTF version is unverified. It follows pinned [JSONValues.cpp](https://github.com/WebKit/WebKit/blob/c55364d53f971f63a709976f168d932d558b8909/Source/WTF/wtf/JSONValues.cpp#L528-L566): the seven short escapes, printable ASCII other than `<` and `>` as is, and every other code unit, surrogates included, as `\u` with four uppercase hex digits, one append per code unit. libSceJsc does not model `StringBuilder` growth, so each append goes through libSceNKWebKit `StringBuilder::appendCharacters` registered in the API bridge; without it the call throws. A null builder, null characters with a nonzero length and unreadable characters throw where WebKit faults. Closes when: compared with a console module.

## remove | `_ZN3WTF8JSONImpl5Value12escapeStringERNS_13StringBuilderENS_10StringViewE` | `ewZNP1Jwrx8` |
