# libSceNKWebKit

## remove 6yS9uNax8IU

## add Functional

- [JSC::PropertyDescriptor::setDescriptor](../../../core/libs/prx/libSceNKWebKit/src/PropertyDescriptor.cpp) (libSceNKWebKit, NID `6yS9uNax8IU`) follows [PropertyDescriptor.cpp](https://github.com/neonmodder123/PS5-WebKit-11.x/blob/80f77e94c63ee7202eec3d4b5b1a021dd89011ba/Source/JavaScriptCore/runtime/PropertyDescriptor.cpp#L150-L172) for every value that is not a `GetterSetter` cell: it stores the attributes without `PropertyAttribute::CustomValue` (`1 << 6`), the value, and `EnumerablePresent | ConfigurablePresent | WritablePresent`, leaving the getter and setter as they were. A cell whose `JSType` byte (offset 5) is `GetterSetterType` (5 in that tree's `JSType.h`) throws through `NotImplemented_nid_no_patch` before changing the descriptor, because the upstream branch needs `GetterSetter::isGetterNull` and `isSetterNull`, which are `jsDynamicCast`s to `NullGetterFunction` and `NullSetterFunction`. An empty `JSValue`, where upstream asserts, throws `std::invalid_argument`. The console's body was not read
