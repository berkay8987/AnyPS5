// Derived from https://github.com/WebKit/WebKit/blob/c55364d53f971f63a709976f168d932d558b8909/Source/WebCore/platform/graphics/GraphicsLayerTransform.cpp
/*
 Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License as published by the Free Software Foundation; either
 version 2 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.

 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
 */

#include "prx/libSceNKWebKit/include/InventoryExports.hpp"
#include "prx/libSceNKWebKit/src/WebCoreTypes.hpp"

#include <cstddef>
#include <new>
#include <stdexcept>

namespace {

struct GraphicsLayerTransform {
    float anchorPoint[3];
    float position[2];
    float size[2];
    bool flattening;
    bool dirty;
    bool childrenDirty;
    WebCore::TransformationMatrix local;
    WebCore::TransformationMatrix children;
    WebCore::TransformationMatrix combined;
    WebCore::TransformationMatrix combinedForChildren;
};

static_assert(sizeof(GraphicsLayerTransform) == 544);
static_assert(alignof(GraphicsLayerTransform) == 16);
static_assert(offsetof(GraphicsLayerTransform, position) == 12);
static_assert(offsetof(GraphicsLayerTransform, size) == 20);
static_assert(offsetof(GraphicsLayerTransform, flattening) == 28);
static_assert(offsetof(GraphicsLayerTransform, dirty) == 29);
static_assert(offsetof(GraphicsLayerTransform, childrenDirty) == 30);
static_assert(offsetof(GraphicsLayerTransform, local) == 32);
static_assert(offsetof(GraphicsLayerTransform, children) == 160);
static_assert(offsetof(GraphicsLayerTransform, combined) == 288);
static_assert(offsetof(GraphicsLayerTransform, combinedForChildren) == 416);

void Construct(void* self) {
    if (!self) throw std::invalid_argument("WebCore::GraphicsLayerTransform: null this");
    auto* transform = static_cast<GraphicsLayerTransform*>(self);
    transform->anchorPoint[0] = 0.5f;
    transform->anchorPoint[1] = 0.5f;
    transform->anchorPoint[2] = 0.0f;
    transform->position[0] = 0.0f;
    transform->position[1] = 0.0f;
    transform->size[0] = 0.0f;
    transform->size[1] = 0.0f;
    transform->flattening = true;
    transform->dirty = false;
    transform->childrenDirty = false;
    new (&transform->local) WebCore::TransformationMatrix();
    new (&transform->children) WebCore::TransformationMatrix();
    new (&transform->combined) WebCore::TransformationMatrix();
    new (&transform->combinedForChildren) WebCore::TransformationMatrix();
}

}

extern "C" {

void APS5_VABI _ZN7WebCore22GraphicsLayerTransformC1Ev(void* self) {
    Construct(self);
}

void APS5_VABI _ZN7WebCore22GraphicsLayerTransformC2Ev(void* self) {
    Construct(self);
}

}
