#pragma once

// Implementations of various convex hull algorithms
// using the C++ Standard Library.
// For clarity, the implementations do not check for
// duplicate or collinear points.

#include <algorithm>
#include <iostream>
#include <vector>

using namespace std;


// The z-value of the cross product of segments
// (a, b) and (a, c). Positive means c is Ccw
// from (a, b), negative cw. Zero means its collinear.
float Ccw(const glm::vec2& a, const glm::vec2& b, const glm::vec2& c) { return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x); }

// Returns true if a is lexicographically before b.
bool IsLeftOf(const glm::vec2& a, const glm::vec2& b) { return (a.x < b.x || (a.x == b.x && a.y < b.y)); }

// Used to sort points in Ccw order about a pivot.
struct ccwSorter
{
    const glm::vec2& pivot;

    ccwSorter(const glm::vec2& inPivot) : pivot(inPivot) {}

    bool operator()(const glm::vec2& a, const glm::vec2& b) { return Ccw(pivot, a, b) < 0; }
};

// The length of segment (a, b).
float Length(const glm::vec2& a, const glm::vec2& b) { return sqrt((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y)); }

// The unsigned distance of p from segment (a, b).
float Distance(const glm::vec2& a, const glm::vec2& b, const glm::vec2& p)
{
    return fabs((b.x - a.x) * (a.y - p.y) - (b.y - a.y) * (a.x - p.x)) / Length(a, b);
}

// Returns the index of the farthest glm::vec2 from segment (a, b).
size_t GetFarthest(const glm::vec2& a, const glm::vec2& b, const vector<glm::vec2>& v)
{
    size_t idxMax = 0;
    float distMax = Distance(a, b, v[idxMax]);

    for (size_t i = 1; i < v.size(); ++i)
    {
        float distCurr = Distance(a, b, v[i]);
        if (distCurr > distMax)
        {
            idxMax = i;
            distMax = distCurr;
        }
    }

    return idxMax;
}

// The gift-wrapping algorithm for convex hull.
// https://en.wikipedia.org/wiki/Gift_wrapping_algorithm
vector<glm::vec2> GiftWrapping(vector<glm::vec2> v)
{
    // Move the leftmost glm::vec2 to the beginning of our vector.
    // It will be the first glm::vec2 in our convext hull.
    swap(v[0], *min_element(v.begin(), v.end(), IsLeftOf));

    vector<glm::vec2> hull;
    // Repeatedly find the first Ccw glm::vec2 from our last hull glm::vec2
    // and put it at the front of our array.
    // Stop when we see our first glm::vec2 again.
    do
    {
        hull.push_back(v[0]);
        swap(v[0], *min_element(v.begin() + 1, v.end(), ccwSorter(v[0])));
    } while (v[0].x != hull[0].x && v[0].y != hull[0].y);

    return hull;
}

// The Graham scan algorithm for convex hull.
// https://en.wikipedia.org/wiki/Graham_scan
vector<glm::vec2> GrahamScan(vector<glm::vec2> v)
{
    // Put our leftmost glm::vec2 at index 0
    swap(v[0], *min_element(v.begin(), v.end(), IsLeftOf));

    // Sort the rest of the points in counter-clockwise order
    // from our leftmost glm::vec2.
    sort(v.begin() + 1, v.end(), ccwSorter(v[0]));

    // Add our first three points to the hull.
    vector<glm::vec2> hull;
    auto it = v.begin();
    hull.push_back(*it++);
    hull.push_back(*it++);
    hull.push_back(*it++);

    while (it != v.end())
    {
        // Pop off any points that make a convex angle with *it
        while (Ccw(*(hull.rbegin() + 1), *(hull.rbegin()), *it) >= 0)
        {
            hull.pop_back();
        }
        hull.push_back(*it++);
    }

    return hull;
}

// The monotone chain algorithm for convex hull.
vector<glm::vec2> MonotoneChain(vector<glm::vec2> v)
{
    // Sort our points in lexicographic order.
    sort(v.begin(), v.end(), IsLeftOf);

    // Find the lower half of the convex hull.
    vector<glm::vec2> lower;
    for (auto it = v.begin(); it != v.end(); ++it)
    {
        // Pop off any points that make a convex angle with *it
        while (lower.size() >= 2 && Ccw(*(lower.rbegin() + 1), *(lower.rbegin()), *it) >= 0)
        {
            lower.pop_back();
        }
        lower.push_back(*it);
    }

    // Find the upper half of the convex hull.
    vector<glm::vec2> upper;
    for (auto it = v.rbegin(); it != v.rend(); ++it)
    {
        // Pop off any points that make a convex angle with *it
        while (upper.size() >= 2 && Ccw(*(upper.rbegin() + 1), *(upper.rbegin()), *it) >= 0)
        {
            upper.pop_back();
        }
        upper.push_back(*it);
    }

    vector<glm::vec2> hull;
    hull.insert(hull.end(), lower.begin(), lower.end());
    // Both hulls include both endpoints, so leave them out when we
    // append the upper hull.
    hull.insert(hull.end(), upper.begin() + 1, upper.end() - 1);
    return hull;
}

// Recursive call of the quickhull algorithm.
void QuickHull(const vector<glm::vec2>& v, const glm::vec2& a, const glm::vec2& b, vector<glm::vec2>& hull)
{
    if (v.empty())
    {
        return;
    }

    glm::vec2 f = v[GetFarthest(a, b, v)];

    // Collect points to the left of segment (a, f)
    vector<glm::vec2> left;
    for (auto p : v)
    {
        if (Ccw(a, f, p) > 0)
        {
            left.push_back(p);
        }
    }
    QuickHull(left, a, f, hull);

    // Add f to the hull
    hull.push_back(f);

    // Collect points to the left of segment (f, b)
    vector<glm::vec2> right;
    for (auto p : v)
    {
        if (Ccw(f, b, p) > 0)
        {
            right.push_back(p);
        }
    }
    QuickHull(right, f, b, hull);
}

// QuickHull algorithm.
// https://en.wikipedia.org/wiki/QuickHull
vector<glm::vec2> QuickHull(const vector<glm::vec2>& v)
{
    if (v.empty()) return {};
    vector<glm::vec2> hull;

    // Start with the leftmost and rightmost points.
    glm::vec2 a = *min_element(v.begin(), v.end(), IsLeftOf);
    glm::vec2 b = *max_element(v.begin(), v.end(), IsLeftOf);

    // Split the points on either side of segment (a, b)
    vector<glm::vec2> left, right;
    for (auto p : v)
    {
        Ccw(a, b, p) > 0 ? left.push_back(p) : right.push_back(p);
    }

    // Be careful to add points to the hull
    // in the correct order. Add our leftmost glm::vec2.
    hull.push_back(a);

    // Add hull points from the left (top)
    QuickHull(left, a, b, hull);

    // Add our rightmost glm::vec2
    hull.push_back(b);

    // Add hull points from the right (bottom)
    QuickHull(right, b, a, hull);

    return hull;
}

vector<glm::vec2> GetPoints()
{
    vector<glm::vec2> v;

    const float lo = -100.0;
    const float hi = 100.0;

    for (int i = 0; i < 100; ++i)
    {
        float x = lo + static_cast<float>(rand()) / static_cast<float>(RAND_MAX / (hi - lo));

        float y = lo + static_cast<float>(rand()) / static_cast<float>(RAND_MAX / (hi - lo));

        v.push_back(glm::vec2(x, y));
    }

    return v;
}

void print(const vector<glm::vec2>& v)
{
    for (auto p : v)
    {
        cout << p.x << ", " << p.y << endl;
    }
}