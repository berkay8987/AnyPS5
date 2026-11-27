# libSceNKWebKit

## remove 6yS9uNax8IU

## add Functional

- [JSC::PropertyDescriptor::setDescriptor](../../../core/libs/prx/libSceNKWebKit/src/PropertyDescriptor.cpp) (libSceNKWebKit, NID `6yS9uNax8IU`) - a `GetterSetter` cell (`JSType` byte at offset 5 equal to 5) throws through `NotImplemented_nid_no_patch` before changing the descriptor, because that branch needs `GetterSetter::isGetterNull` and `isSetterNull` (`jsDynamicCast`s to `NullGetterFunction` and `NullSetterFunction`); an empty `JSValue`, where upstream asserts, throws `std::invalid_argument`; the console's body was not read. Other values follow [PropertyDescriptor.cpp](https://github.com/neonmodder123/PS5-WebKit-11.x/blob/80f77e94c63ee7202eec3d4b5b1a021dd89011ba/Source/JavaScriptCore/runtime/PropertyDescriptor.cpp#L150-L172): attributes without `CustomValue` (`1 << 6`), the value, and all three presence bits, with the getter and setter left as they were
