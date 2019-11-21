/*
 * Copyright 2013 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "tests/PathOpsExtendedTest.h"

DEF_TEST(PathOpsInverse, reporter) {
    const SkPathDirection dirs[] = {SkPathDirection::kCW, SkPathDirection::kCCW};
    SkPath one, two;
    int testCount = 0;
    for (int op = kDifference_SkPathOp; op <= kReverseDifference_SkPathOp; ++op) {
        for (int oneFill = SkPath::kWinding_FillType; oneFill <= SkPath::kInverseEvenOdd_FillType;
                    ++oneFill) {
            for (auto oneDir : dirs) {
                one.reset();
                one.setFillType((SkPath::FillType) oneFill);
                one.addRect(0, 0, 6, 6, oneDir);
                for (int twoFill = SkPath::kWinding_FillType;
                        twoFill <= SkPath::kInverseEvenOdd_FillType; ++twoFill) {
                    for (auto twoDir : dirs) {
                        two.reset();
                        two.setFillType((SkPath::FillType) twoFill);
                        two.addRect(3, 3, 9, 9, twoDir);
                        SkString testName;
                        testName.printf("inverseTest%d", ++testCount);
                        testPathOp(reporter, one, two, (SkPathOp) op, testName.c_str());
                    }
                }
            }
        }
    }
}
