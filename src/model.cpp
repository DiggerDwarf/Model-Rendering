

#include "header.hpp"

#define VERTEX 0
#define NORMAL 1
#define UV     2


Mat3 operator*(Mat3 m1, Mat3 m2)
{
    return Mat3{
        (m1.a1*m2.a1) + (m1.a2*m2.b1) + (m1.a3*m2.c1),
        (m1.a1*m2.a2) + (m1.a2*m2.b2) + (m1.a3*m2.c2),
        (m1.a1*m2.a3) + (m1.a2*m2.b3) + (m1.a3*m2.c3),
        (m1.b1*m2.a1) + (m1.b2*m2.b1) + (m1.b3*m2.c1),
        (m1.b1*m2.a2) + (m1.b2*m2.b2) + (m1.b3*m2.c2),
        (m1.b1*m2.a3) + (m1.b2*m2.b3) + (m1.b3*m2.c3),
        (m1.c1*m2.a1) + (m1.c2*m2.b1) + (m1.c3*m2.c1),
        (m1.c1*m2.a2) + (m1.c2*m2.b2) + (m1.c3*m2.c2),
        (m1.c1*m2.a3) + (m1.c2*m2.b3) + (m1.c3*m2.c3),
    };
}
void operator*=(Mat3& m1, Mat3 m2)
{
    m1 = m1*m2;
}
coord operator*(Mat3 m, coord c)
{
    return coord{
        (c[0]*m.a1) +(c[1]*m.b1) +(c[2]*m.c1),
        (c[0]*m.a2) +(c[1]*m.b2) +(c[2]*m.c2),
        (c[0]*m.a3) +(c[1]*m.b3) +(c[2]*m.c3),
    };
}
coord operator-(coord c1, coord c2)
{
    return coord{c1[0]-c2[0], c1[1]-c2[1], c1[2]-c2[2]};
}
coord operator+(coord c1, coord c2)
{
    return coord{c1[0]+c2[0], c1[1]+c2[1], c1[2]+c2[2]};
}
coord operator/(coord c, float n)
{
    return coord{c[0]/n, c[1]/n, c[2]/n};
}
coord operator*(coord c, float n)
{
    return coord{c[0]*n, c[1]*n, c[2]*n};
}
coord cross(coord c1, coord c2)
{
    return coord{
        c1[1]*c2[2] - c1[2]*c2[1],
        c1[2]*c2[0] - c1[0]*c2[2],
        c1[0]*c2[1] - c1[1]*c2[0]
    };
}
float dot(coord c1, coord c2)
{
    return (c1[0]*c2[0])+(c1[1]*c2[1])+(c1[2]*c2[2]);
}
Mat3 angles_to_matrix(float angles[2])
{
    return Mat3{
            cosf(angles[0]), 0,-sinf(angles[0]),
            0,                     1, 0,
            sinf(angles[0]), 0, cosf(angles[0])
    } *Mat3{
            1, 0,                     0,
            0, cosf(angles[1]), sinf(angles[1]),
            0,-sinf(angles[1]), cosf(angles[1])
    };
}
coord normalize(coord c)
{
    return c/sqrt((c[0]*c[0])+(c[1]*c[1])+(c[2]*c[2]));
}


struct File     // File convenice
{
private:
    FILE* file;             // Actual file object
    char current;           // Current char in file at pos
    fpos_t pos;             // Position in file
    bool isOpen = false;    // Is a file open in this struct
public:
    // Advance in the file
    File& operator++()
    {
        if (!this->isOpen) return *this;
        pos++;
        fsetpos(this->file, &this->pos);
        if ((this->current = fgetc(this->file)) == EOF)
        {
            this->current = '\0';
        }
        fsetpos(this->file, &this->pos);
        return *this;
    }
    File  operator++(int)
    {
        return ++*this;
    }
    // Go back in the file
    File& operator--()
    {
        if (!this->isOpen) return *this;
        pos--;
        fsetpos(this->file, &this->pos);
        if ((this->current = fgetc(this->file)) == EOF)
        {
            this->current = '\0';
        }
        fsetpos(this->file, &this->pos);
        return *this;
    }
    File  operator--(int)
    {
        return --*this;
    }
    // Fetch current character
    char  operator()()
    {
        return this->isOpen ? this->current : '\0';
    }
    // Open a file
    void open(const char* fileName)
    {
        this->isOpen = true;
        this->file = fopen(fileName, "rb");
        fgetpos(this->file, &this->pos);
        this->current = fgetc(this->file);
        fsetpos(this->file, &this->pos);
    }
    // Close the file
    void close()
    {
        this->isOpen = false;
        fclose(this->file);
    }
};

float read_float(const char* (& data));
uint read_uint(const char* (& data));
float read_float(File& data);
uint read_uint(File& data);


// Reads and outputs the value of a float from a char buffer
// The char pointer must be located on the first character of the float
// The char pointer will be advanced to the character after the float
float read_float(const char* (& data))
{
    float result = 0;
    float sign = +1;
    float where = 0;

    while (*data != ' ' && *data != '\r' && *data != '\n' && *data != '\0')
    {
        if(*data == '-')
        {
            sign = -1;
        }
        else if (*data == '.')
        {
            where = 1;
        }
        else
        {
            if (where == 0) result *= 10;
            result += (*data - '0') / powf(10.F, where);
            if (where != 0) where += 1;
        }
        
        data++;
    }

    result *= sign;

    return result;
}

// Reads and outputs the value of a uint from a char buffer
// The char pointer must be located on the first character of the uint
// The char pointer will be advanced to the character after the uint
uint read_uint(const char* (& data))
{
    float result = 0;

    while (*data != ' ' && *data != '\r' && *data != '\n' && *data != '\0')
    {
        result *= 10;
        result += *data - '0';
        data++;
    }

    return result;
}


// Reads and outputs the value of a float from a file
// The reading position must be located on the first character of the float
// The reading position will be advanced to the character after the float
float read_float(File &data)
{
    float result = 0;
    float sign = +1;
    float where = 0;
    int expval = 0;

    while (data() != ' ' && data() != '\r' && data() != '\n' && data() != '\0')
    {
        if(data() == '-')
        {
            sign = -1;
        }
        else if (data() == '.')
        {
            where = 1;
        }
        else
        {
            if (where == 0) result *= 10;
            result += (data() - '0') / powf(10.F, where);
            if (where != 0) where += 1;
        }
        
        data++;

        if (data() == 'e' || data() == 'E')
        {
            data++;
            expval = (data() == '-') ? -1 : +1;
            data++;
            expval *= static_cast<int>(read_uint(data));
        }
        
    }

    result *= sign;
    result *= pow(10, expval);

    return result;
}

// Reads and outputs the value of a uint from a file
// The reading position must be located on the first character of the uint
// The reading position will be advanced to the character after the uint
uint read_uint(File &data)
{
    float result = 0;

    while (data() >= '0' && data() <= '9')
    {
        result *= 10;
        result += data() - '0';
        data++;
    }

    return result;
}


// Extracts model data in the wavefront obj format from a char buffer
// If the size of the data chunk is unknown,
// leave blank and the buffer will be read up to the first null-terminator
// Currently supported :
//  - geometry vertices
//  - normals
//  - face indexed by geometry vertices only
Model get_model_info(const char* dataStart, size_t dataSize)
{
    Model model;

    const char* charPtr = dataStart;

    coord tempVertex;
    uint must;
    uint optional;
    float default_;

    std::array<uint, 3> tempFace;

    std::vector<coord>* target;

    while (true)
    {
        if (charPtr > dataStart + dataSize) break;
        else if (*charPtr == '\0') break;
        else if (*charPtr == '#')
        {
            while (not(*charPtr == '\n' || *charPtr == '\r' || *charPtr == '\0'))
            {
                charPtr++;
            }
        }
        else if (*charPtr == 'v')
        {
            charPtr++;
            if (*charPtr == ' ') target = &(model.vertices);
            else if (*charPtr == 'n') target = &(model.normals);
            else if (*charPtr == 't') target = &(model.textureCoords);
            charPtr++;
            for (int i = 0; i < 3; i++)
            {
                tempVertex[i] = read_float(charPtr);
                charPtr++;
            }
            charPtr--;
            target->push_back(tempVertex);
        }
        else if (*charPtr == 'f')
        {
            charPtr += 2;
            for (int i = 0; i < 3; i++)
            {
                tempFace[i] = read_uint(charPtr)-1;
                charPtr++;
            }
            charPtr--;
            model.faces.push_back({tempFace, {0, 0, 0}, {0, 0, 0}});
        }
        charPtr++;
    }
    
    // std::cout << model.vertices.size() << " vertices have been found.\n";
    // std::cout << model.faces.size() << " triangles have been found.\n";

    return model;
}

// Extracts model data in the wavefront obj format from a file path
//
// Currently supported :
//  - geometry vertices
//  - normals
//  - face indexed by geometry vertices only
Model get_model_info_file(const char* fileName, CONDITION interpretNormals)
{
    Model model;

    File data;
    data.open(fileName);

    std::array<float, 4> tempVertex;
    coord tempNormal;
    coord tempTexture;

    uint type;

    uint must;
    uint optional;
    float default_;

    face tempFace;

    bool isOffset = false;

    while (true)
    {
        if (data() == '\0') [[unlikely]] break;
        else if (data() == '#' || data() == 'o' || data() == 's' || data() == 'g' || data() == 'u' || data() == 'm') [[unlikely]]
        {
            while (not(data() == '\n' || data() == '\r' || data() == '\0'))
            {
                data++;
            }
        }
        else if (data() == 'v')
        {
            data++;
            if (data() == ' ')          // Geometrix vertex
            {
                type = VERTEX;
                must = 3;
                optional = 1;
                default_ = 1.0F;
                // Don't advance further; already on required position
            }
            else if (data() == 'n')     // Normal vector
            {
                type = NORMAL;
                must = 3;
                optional = 0;
                // no default since no optional
                data++;
            }
            else if (data() == 't')     // Texture coordinate
            {
                type = UV;
                must = 1;
                optional = 2;
                default_ = 0.0F;
                data++;
            }
            data++;
            for (int i = 0; i < must; i++)                  // Read required information
            {
                if (type == VERTEX)         tempVertex[i]  = read_float(data);
                else if (type == NORMAL)    tempNormal[i]  = read_float(data);
                else if (type == UV)        tempTexture[i] = read_float(data);
                data++;
            }
            for (int i = must; i < must + optional; i++)    // Read optional information
            {
                if ((data() > '0' && data() < '9') || data() == '-' || data() == '+')       // if still on a number, read it
                {
                    if (type == VERTEX)         tempVertex[i]  = read_float(data);
                    // Normals have no optional
                    else if (type == UV)        tempTexture[i] = read_float(data);
                    data++;
                }
                else                                    // else put default value
                {
                    if (type == VERTEX)         tempVertex[i]  = default_;
                    // Normals have no optional
                    else if (type == UV)        tempTexture[i] = default_;
                }
                
            }

            data--;
            if (type == VERTEX)         model.vertices.push_back({tempVertex[0], tempVertex[1], tempVertex[2]});
            else if (type == NORMAL)    model.normals.push_back(tempNormal);
            else if (type == UV)        model.textureCoords.push_back(tempTexture);
        }
        else if (data() == 'f')
        {
            data++;
            data++;
            for (int i = 0; i < 3; i++)     // for vertex in face
            {
                for (int j = 0; j < 3; j++)     // for information in vertex
                {
                    if (data() == '-') [[unlikely]]
                    {
                        isOffset = true;
                        data++;
                        tempFace[j][i] == model.vertices.size() - read_uint(data);
                    }
                    else
                    {
                        if (data() < '0' or data() > '9')
                        {
                            if (data() == '/') 
                            {
                                data++;
                                // j++;
                            }
                            else
                            {
                                tempFace[j][i] = 0;
                                continue;
                            }
                        }
                        
                        tempFace[j][i] = read_uint(data)-1;
                    }
                }       // end for info in vertex
                
                data++;
            }       // end for vertex in face
            data--;
            model.faces.push_back(tempFace);
        }
        data++;
    }

    data.close();

    if ((model.normals.empty() && (interpretNormals == DO_IF)) || (interpretNormals == DO_WHATEVER))
    {
        coord normal;
        for (std::vector<face>::iterator face_it = model.faces.begin(); face_it != model.faces.end(); face_it++)
        {
            normal = cross(model.vertices[(*face_it)[0][0]] - model.vertices[(*face_it)[0][1]], model.vertices[(*face_it)[0][1]] - model.vertices[(*face_it)[0][2]]);
            model.normals.push_back(normal);
            face_it->operator[](1) = {(uint)model.normals.size()-1, (uint)model.normals.size()-1, (uint)model.normals.size()-1};
        }
    }

    for (coord& normal : model.normals)
    {
        normal = normalize(normal);
    }
    
    

    return model;
}
