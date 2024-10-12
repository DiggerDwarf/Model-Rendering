#ifndef MODEL_INCLUDED
#define MODEL_INCLUDED


#include <iostream> // I/O
#include <cmath>    // Meth
#include <vector>   // Variable-length storage
#include <list>     // double-linked list; can keep pointers stable
#include <array>    // Static-length storage

// Shortcuts
typedef unsigned int uint;
typedef std::array<float, 3> coord;
typedef std::array<std::array<uint, 3>, 3> face;

#define SFML_STATIC
#include <SFML/Graphics.hpp>    // Graphics


struct Mat3     // 3x3 Matrix data
{
    float a1;   // Top left
    float a2;   // Top middle
    float a3;   // Top right
    float b1;   // Middle left
    float b2;   // Center
    float b3;   // Middle right
    float c1;   // Bottom left
    float c2;   // Bottom middle
    float c3;   // Bottom right
};

Mat3 operator*(Mat3 m1, Mat3 m2);
void operator*=(Mat3& m1, Mat3 m2);
coord operator*(Mat3 m, coord c);
coord operator-(coord c1, coord c2);
coord operator+(coord c1, coord c2);
coord cross(coord c1, coord c2);
float dot(coord c1, coord c2);
Mat3 angles_to_matrix(float angles[2]);
coord normalize(coord c);


struct Model    // Model data storage unit
{
    coord position;
    std::vector<coord> vertices;        // Geometric vertex data in x, y, z form
    std::vector<coord> normals;         // Vertex normal vectors
    std::vector<coord> textureCoords;   // Texture coordinates
    std::vector<face>  faces;           // Faces vertex indices `(vertex[3], normals[3], uvs[3])`
};

struct File;    // File conveniency structure

// Reads and outputs the value of a float from a char buffer
// The char pointer must be located on the first character of the float
// The char pointer will be advanced to the character after the float
float read_float(const char* (& data));
// Reads and outputs the value of a uint from a char buffer
// The char pointer must be located on the first character of the uint
// The char pointer will be advanced to the character after the uint
uint read_uint(const char* (& data));
// Reads and outputs the value of a float from a file
// The reading position must be located on the first character of the float
// The reading position will be advanced to the character after the float
float read_float(File& data);
// Reads and outputs the value of a uint from a file
// The reading position must be located on the first character of the uint
// The reading position will be advanced to the character after the uint
uint read_uint(File& data);

// Extracts model data in the wavefront obj format from a char buffer
// If the size of the data chunk is unknown,
// leave blank and the buffer will be read up to the first null-terminator
// Currently supported :
//  - geometry vertices
//  - normals
//  - face indexed by geometry vertices only
Model get_model_info(const char* dataStart, size_t dataSize = (size_t)INFINITY);

typedef enum {
    DONT = 0,
    DO_IF = 1,
    DO_WHATEVER
} CONDITION;

// Extracts model data in the wavefront obj format from a file path
//
// Currently supported :
//  - geometry vertices
//  - normals
//  - UVs
//  - face indexed by geometry vertices only
Model get_model_info_file(const char* fileName, CONDITION interpretNormals = DO_IF);

#endif