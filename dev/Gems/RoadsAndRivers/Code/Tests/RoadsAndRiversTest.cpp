
#include "StdAfx.h"

#include <AzTest/AzTest.h>
#include <Tests/TestTypes.h>

#include "RoadRiverCommon.h"

namespace UnitTest
{
    class WidthInterpolator
        : public AllocatorsFixture
    {
    public:

        void EdgeCases()
        {
            const float tolerance = 1e-4;
            {
                RoadsAndRivers::RoadWidthInterpolator m_interpolator;
                // empty interpolator returns 0.0f
                EXPECT_NEAR(m_interpolator.GetWidth(0.0f), 0.0f, tolerance);
            }

            {
                RoadsAndRivers::RoadWidthInterpolator m_interpolator;
                const float width = 2.0f;
                m_interpolator.InsertDistanceWidthKeyFrame(1.0f, width);

                // interpolator with 1 key frame always returns this keyframe
                EXPECT_NEAR(m_interpolator.GetWidth(0.0f), width, tolerance);
                EXPECT_NEAR(m_interpolator.GetWidth(1.0f), width, tolerance);
                EXPECT_NEAR(m_interpolator.GetWidth(2.0f), width, tolerance);
            }

            {
                RoadsAndRivers::RoadWidthInterpolator m_interpolator;

                m_interpolator.InsertDistanceWidthKeyFrame(1.0f, 2.0f);
                m_interpolator.InsertDistanceWidthKeyFrame(2.0f, 10.0f);

                // distance more than edge points yields edge points
                EXPECT_NEAR(m_interpolator.GetWidth(0.0f), 2.0f, tolerance);
                EXPECT_NEAR(m_interpolator.GetWidth(3.0f), 10.0f, tolerance);
            }
        }

        void Interpolation()
        {
            // Inserting sorted keys
            {
                RoadsAndRivers::RoadWidthInterpolator m_interpolator;

                m_interpolator.InsertDistanceWidthKeyFrame(1.0f, 1.0f);
                m_interpolator.InsertDistanceWidthKeyFrame(2.0f, 10.0f);
                m_interpolator.InsertDistanceWidthKeyFrame(3.0f, 5.0f);

                auto res = m_interpolator.GetWidth(1.5f);
                EXPECT_GT(res, 1.0f);
                EXPECT_LT(res, 10.0f);

                res = m_interpolator.GetWidth(2.5f);
                EXPECT_LT(res, 10.0f);
                EXPECT_GT(res, 5.0f);

                EXPECT_NEAR(m_interpolator.GetWidth(2.0f), 10.0f, 1e-4);
            }

            // Inserting unsorted keys
            {
                RoadsAndRivers::RoadWidthInterpolator m_interpolator;

                m_interpolator.InsertDistanceWidthKeyFrame(3.0f, 5.0f);
                m_interpolator.InsertDistanceWidthKeyFrame(1.0f, 1.0f);
                m_interpolator.InsertDistanceWidthKeyFrame(2.0f, 10.0f);

                auto res = m_interpolator.GetWidth(1.5f);
                EXPECT_GT(res, 1.0f);
                EXPECT_LT(res, 10.0f);

                res = m_interpolator.GetWidth(2.5f);
                EXPECT_LT(res, 10.0f);
                EXPECT_GT(res, 5.0f);

                EXPECT_NEAR(m_interpolator.GetWidth(2.0f), 10.0f, 1e-4);
            }
        }
    };

    TEST_F(WidthInterpolator, EdgeCases)
    {
        EdgeCases();
    }

    TEST_F(WidthInterpolator, Interpolation)
    {
        Interpolation();
    }

    AZ_UNIT_TEST_HOOK();
}
