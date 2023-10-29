#pragma once

#include <vendor/glm_include.hpp>

namespace imp::detail
{
    inline
    glm::vec3 Reorthogonalize(glm::vec3 v, glm::vec3 other)
    {
        return glm::normalize(v - glm::dot(v, other) * other);
    }

// -----------------------------------------------------------------------------
//                               Encode Normals
// -----------------------------------------------------------------------------

    inline
    glm::vec3 SignedOctEncode(glm::vec3 n)
    {
        glm::vec3 outN;

        n /= (glm::abs(n.x) + glm::abs(n.y) + glm::abs(n.z));

        outN.y = n.y *  0.5f + 0.5f;
        outN.x = n.x *  0.5f + outN.y;
        outN.y = n.x * -0.5f + outN.y;

        outN.z = glm::clamp(n.z * FLT_MAX, 0.f, 1.f);
        return outN;
    }

// -----------------------------------------------------------------------------
//                              Decode Normals
// -----------------------------------------------------------------------------

    inline
    glm::vec3 SignedOctDecode(glm::vec3 n)
    {
        glm::vec3 outN;

        outN.x = (n.x - n.y);
        outN.y = (n.x + n.y) - 1.f;
        outN.z = n.z * 2.f - 1.f;
        outN.z = outN.z * (1.f - glm::abs(outN.x) - glm::abs(outN.y));

        outN = glm::normalize(outN);
        return outN;
    }

// -----------------------------------------------------------------------------
//                              Encode Tangents
// -----------------------------------------------------------------------------

    inline
    float EncodeDiamond(glm::vec2 p)
    {
        // Project to the unit diamond, then to the x-axis.
        float x = p.x / (glm::abs(p.x) + glm::abs(p.y));

        // Contract the x coordinate by a factor of 4 to represent all 4 quadrants in
        // the unit range and remap
        float pySign = glm::sign(p.y);
        return -pySign * 0.25f * x + 0.5f + pySign * 0.25f;
    }

    // Given a normal and tangent vector, encode the tangent as a single float that can be
    // subsequently quantized.
    inline
    float EncodeTangent(glm::vec3 normal, glm::vec3 tangent)
    {
        // First, find a canonical direction in the tangent plane
        glm::vec3 t1;
        if (glm::abs(normal.y) > glm::abs(normal.z))
        {
            // Pick a canonical direction orthogonal to n with z = 0
            t1 = glm::vec3(normal.y, -normal.x, 0.f);
        }
        else
        {
            // Pick a canonical direction orthogonal to n with y = 0
            t1 = glm::vec3(normal.z, 0.f, -normal.x);
        }
        t1 = glm::normalize(t1);

        // Construct t2 such that t1 and t2 span the plane
        glm::vec3 t2 = glm::cross(t1, normal);

        // Decompose the tangent into two coordinates in the canonical basis
        glm::vec2 packedTangent = glm::vec2(glm::dot(tangent, t1), glm::dot(tangent, t2));

        // Apply our diamond encoding to our two coordinates
        return EncodeDiamond(packedTangent);
    }

// -----------------------------------------------------------------------------
//                              Decode Tangents
// -----------------------------------------------------------------------------

    inline
    glm::vec2 DecodeDiamond(float p)
    {
        glm::vec2 v;

        // Remap p to the appropriate segment on the diamond
        float pSign = glm::sign(p - 0.5f);
        v.x = -pSign * 4.f * p + 1.f + pSign * 2.f;
        v.y = pSign * (1.f - glm::abs(v.x));

        // Normalization extends the point on the diamond back to the unit circle
        return glm::normalize(v);
    }

    inline
    glm::vec3 DecodeTangent(glm::vec3 normal, float diamondTangent)
    {
        // As in the encode step, find our canonical tangent basis span(t1, t2)
        glm::vec3 t1;
        if (glm::abs(normal.y) > glm::abs(normal.z)) {
            t1 = glm::vec3(normal.y, -normal.x, 0.f);
        } else {
            t1 = glm::vec3(normal.z, 0.f, -normal.x);
        }
        t1 = glm::normalize(t1);

        glm::vec3 t2 = glm::cross(t1, normal);

        // Recover the coordinates used with t1 and t2
        glm::vec2 packedTangent = DecodeDiamond(diamondTangent);

        return packedTangent.x * t1 + packedTangent.y * t2;
    }
}