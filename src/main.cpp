//Start of the obj_render project 

#include "header.hpp"

#include <iostream>

#include <thread>
#include <windows.h>
#include <commdlg.h>

// Initial window size
#define WIN_WIDTH  1920.F
#define WIN_HEIGHT 1080.F

// Movement and rotation speed through key inputs (units/s)
#define MOV_SPEED 2.5F
#define ROT_SPEED 1.5F

// Mouse drag sensitivity
#define SENSITIVITY 0.002

sf::Color get_diffuse(coord vertex, coord normal, coord lightSource)
{
    float coeff = std::max(dot(normal, normalize(lightSource - vertex)), 0.0F);
    return sf::Color(0xFF*coeff, 0xFF*coeff, 0xFF*coeff);
}

// Returns wether a projected line segment is contained within the view frustum and must be drawed
bool can_be_drawed(sf::Vertex* vertices, int nb_vertices)
{
    for (int i = 0; i < nb_vertices; i++)
    {
        if ((vertices[i].position.x <= 1.1) && (vertices[i].position.x >= -1.1) && \
            (vertices[i].position.y <= 1.1) && (vertices[i].position.y >= -1.1)) return true;
    }
    return false;
}

bool can_be_drawed(sf::Vector2f vertex)
{
    return ((vertex.x <= 1) && (vertex.x >= -1) && (vertex.y <= 1) && (vertex.y >= -1));
}

struct Camera   // Camera data storage unit
{
    coord position;         // Position of the camera
    float angle[2];         // Angles of the camera (xz plane angle then up-down angle)
    float fovx;             // Fov of the camera
    float fovy;             // Please set this value to camera.fovx * (window.height / window.width)
    Mat3 rotationMatrix;    // Rotation matrix of the camera, to be updated after each camera angle change
};

struct Object   // Object data storage unit
{
    coord position;             // Object position
    float angle[2];             // Object orientation
    Model* model;               // Object model
    const char* objectTag;      // Object tag in the scene
    const char* modelTag;       // Model tag in the scene
    bool* clipMask;             // Buffer for storing wether a vertex should be drawn according to its projected coordinates
    Mat3 rotationMatrix;        // Rotation matrix of the object
};

// A container class for managing multiple Models
// And multiple Objects
struct Scene
{
    std::vector<std::pair<const char*, Model*>> modelList; // list of models and tags
    std::vector<Object> objectList; // list of model tags and tags
    // Add a model to the scene, and give it a unique (or not) tag, which it will be refered by.
    // If multiple objects are given the same tag, only the first one will be taken into account when creating an object.
    void add_model(Model model, const char* modelTag)
    {
        printf("Loading model . . .\n");
        this->modelList.push_back({modelTag, new Model(model)});
        printf("Model loaded !\n");
    }
    // Removes all models with the corresponding tag from the scene
    void remove_model(const char* modelTag)
    {
        for (std::vector<std::pair<const char*, Model*>>::iterator pair = this->modelList.begin(); pair != this->modelList.end(); pair++)
        {
            if (strcmp(modelTag, pair->first) == 0)
            {
                for (std::vector<Object>::iterator itObject = this->objectList.begin(); itObject != this->objectList.end(); itObject++)
                {
                    if (itObject->model == pair->second)
                    {
                        this->objectList.erase(itObject);
                        itObject--;
                    }
                    
                }
                
                delete pair->second;
                this->modelList.erase(pair);
                pair--;
            }
        }
    }
    // Spawns an object with the model corresponding to the given tag at the given location. (or not cuz i dont wanna)
    // As per the models, it will later be interacted with by its tag.
    // An incorrect modelTag will still be set as is, such that if a corresponding model is added afterwards it'll still work as intended.
    void spawn_object(const char* modelTag, const char* objectTag = "default")
    {
        printf("Searching for corresponding model . . .\n");
        for (std::vector<std::pair<const char*, Model*>>::iterator it = this->modelList.begin(); it != this->modelList.end(); it++)
        {
            if (strcmp(it->first, modelTag) == 0)
            {
                this->objectList.push_back({{0, 0, 0}, {0, 0}, it->second, objectTag, modelTag, {}});
                // this->objectList[this->objectList.size() - 1].projectedBuffer = new sf::Vector2f[it->second->vertices.size()];
                this->objectList[this->objectList.size() - 1].clipMask = new bool[it->second->vertices.size()];
                printf("Model found; Object added !\n");
                return;
            }
        }
        throw "No such model found";
    }
    // Remove all objects with given tag
    void remove_object(const char* objectTag)
    {
        for (std::vector<Object>::iterator it = this->objectList.begin(); it != this->objectList.end(); it++)
        {
            if (strcmp(objectTag, it->objectTag) == 0)
            {
                this->objectList.erase(it);
                it--;
            }
        }
    }

    void clear()
    {
        for (std::pair<const char*, Model*> model : this->modelList)
        {
            remove_model(model.first);
        }
    }

    ~Scene()
    {
        this->clear();
    }
};

void ask_load_model(Scene* scene)
{
    char filename[ MAX_PATH ];

    OPENFILENAMEA ofn;
        ZeroMemory( &filename, sizeof( filename ) );
        ZeroMemory( &ofn,      sizeof( ofn ) );
        ofn.lStructSize  = sizeof( ofn );
        ofn.hwndOwner    = NULL;  // If you have a window to center over, put its HANDLE here
        ofn.lpstrFilter  = "Object files\0*.obj\0";
        ofn.lpstrFile    = filename;
        ofn.nMaxFile     = MAX_PATH;
        ofn.lpstrTitle   = "Select a model to open :";
        ofn.Flags        = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;
    
    if (GetOpenFileNameA( &ofn ))
    {
        int nb = scene->modelList.size();
        char* tag = new char[(int)ceil(log10(nb+2))+1];
        sprintf(tag, "%u", nb);
        scene->add_model(get_model_info_file(filename, DO_WHATEVER), tag);
        scene->spawn_object(tag);
    }
    else std::cout << "Error: Could not open model.\n";
}

// Changes the value of the variable given to be between the low and high thresholds
template <typename T>
void clamp(T& val, T low, T high)
{
    val = std::min(std::max(low, val), high);
}


// Projects a coord to screen space [-1,1] relative to a camera
void project_to(Object& object, Camera& camera, std::pair<sf::Vector2f, float>* buffer)
{
    object.rotationMatrix = angles_to_matrix(object.angle);
    for (int i = 0; i < object.model->vertices.size(); i++)
    {
        // Center and align the vertices relative to the camera
        coord proj = camera.rotationMatrix*(object.position - camera.position + (object.rotationMatrix*object.model->vertices[i]));

        // Exclude vertices behind the camera
        if (__signbitf(proj[2]))
        {
            buffer[i] = {sf::Vector2f(NAN, NAN), proj[2]};
            continue;
        }
        
        // Normalize the height relative to the frustum depth
        buffer[i] = {sf::Vector2f(
            proj[0]/(proj[2]*tanf(camera.fovx)),
            proj[1]/(proj[2]*tanf(camera.fovy))
        ), proj[2]};
    }
}

bool compare_depth(const std::pair<std::array<sf::Vertex, 3>, float> elem1, const std::pair<std::array<sf::Vertex, 3>, float> elem2)
{
    return elem1.second > elem2.second;
}

// Handles key inputs and updates the camera
bool Update(sf::RenderWindow& window, Camera& camera, sf::Clock& clock, bool& isMousePressed, sf::Vector2i& mousePos, Scene* scene)
{
    // Get deltaTime to make a smooth experience
    float dt = clock.restart().asSeconds();

    // Cycle through received events
    sf::Event event;
    while (window.pollEvent(event))
    {
        switch (event.type)
        {
        case sf::Event::Closed:
            window.close();
            return false;
        case sf::Event::KeyPressed:
            if (event.key.code == sf::Keyboard::Escape)
            {
                window.close();
                return false;
            }
            else if (event.key.code == sf::Keyboard::O && event.key.control)
            {
                ask_load_model(scene);
            }
            else if (event.key.code == sf::Keyboard::Backspace)
            {
                scene->clear();
            }
            break;
        
        case sf::Event::MouseButtonPressed:
            isMousePressed = true;
            mousePos = sf::Mouse::getPosition();
            break;
        case sf::Event::MouseButtonReleased:
            isMousePressed = false;
            break;
            
        case sf::Event::Resized:
            if (event.size.height > event.size.width)
            {
                std::cout << "nuh uh fucker\n";
                window.close();
                return false;
            }

            camera.fovy = (((float)(event.size.height))/((float)(event.size.width))) * camera.fovx;
            window.setSize(sf::Vector2u(event.size.width, event.size.height));
            break;

        default:
            break;
        }
    }

    // Put exceptional event handling (for debug for instance) here

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::T)) scene->objectList[0].position[0] += 3*dt;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::G)) scene->objectList[0].position[0] -= 3*dt;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::F)) scene->objectList[0].position[1] -= 3*dt;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::H)) scene->objectList[0].position[1] += 3*dt;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::C)) scene->objectList[0].angle[0] -= 1*dt;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::V)) scene->objectList[0].angle[0] += 1*dt;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::B)) scene->objectList[0].angle[1] -= 1*dt;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::N)) scene->objectList[0].angle[1] += 1*dt;

    // End of exceptional event handling

    if (window.hasFocus()) {

    /* Camera movement */ {
        float dm = dt * MOV_SPEED;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) dm *= 5;
        float s = sin(-camera.angle[0]) * dm;
        float c = cos(-camera.angle[0]) * dm;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Z)) { camera.position[0] += s; camera.position[2] += c; }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) { camera.position[0] -= s; camera.position[2] -= c; }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Q)) { camera.position[0] -= c; camera.position[2] += s; }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) { camera.position[0] += c; camera.position[2] -= s; }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) { camera.position[1] += dm; }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::E)) { camera.position[1] -= dm; }
    }

    /* Camera rotation */ { 
        float dr = dt * ROT_SPEED;

        if (isMousePressed)
        {
            sf::Vector2i npos(sf::Mouse::getPosition());
            sf::Vector2i moffset(mousePos - npos);
            if (sf::Mouse::isButtonPressed(sf::Mouse::Right)) moffset = -moffset;
            mousePos = npos;
            camera.angle[0] -= moffset.x * SENSITIVITY;
            camera.angle[1] -= moffset.y * SENSITIVITY;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::L)) { camera.angle[0] -= dr; }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::J)) { camera.angle[0] += dr; }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::K)) { camera.angle[1] -= dr; }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::I)) { camera.angle[1] += dr; }
    }

    // Lock the up-down rotation to not look behind upside down
    clamp<float>(camera.angle[1], (float)-M_PI_2, (float)M_PI_2);

    // Update the camera's rotation matrix
    camera.rotationMatrix = angles_to_matrix(camera.angle);

    } // if has focus end
    return true;
}

// Render a single Model to the window using a camera as viewpoint
void Render(sf::RenderWindow& window, Scene* scene, Camera& camera)
{
    // Clear the previous frame
    window.clear();

    std::pair<sf::Vector2f, float>* vertexBuffer;
    std::vector<std::pair<std::array<sf::Vertex, 3>, float>> triangleBuffer;

    for (Object& object : scene->objectList)
    {
        if (object.model == nullptr)
        {
            continue;
        }

        // Project the vertices to the camera's screen space and store them in the model's buffer
        // Then construct the clip mask
        vertexBuffer = new std::pair<sf::Vector2f, float>[object.model->vertices.size()];
        project_to(object, camera, vertexBuffer);
        for (int i = 0; i < object.model->vertices.size(); i++)
        {
            object.clipMask[i] = can_be_drawed(vertexBuffer[i].first);
        }

        std::array<sf::Vertex, 3> triangle;

        for (std::array<std::array<uint, 3>, 3>& face : object.model->faces)
        {
            if (object.clipMask[face[0][0]] || object.clipMask[face[0][1]] || object.clipMask[face[0][2]])
            {
                triangle[0] = sf::Vertex(
                    vertexBuffer[face[0][0]].first,
                    get_diffuse((object.rotationMatrix*object.model->vertices[face[0][0]]) + object.position, object.rotationMatrix*object.model->normals[face[1][0]], camera.position)
                );
                triangle[1] = sf::Vertex(
                    vertexBuffer[face[0][1]].first,
                    get_diffuse((object.rotationMatrix*object.model->vertices[face[0][1]]) + object.position, object.rotationMatrix*object.model->normals[face[1][1]], camera.position)
                );
                triangle[2] = sf::Vertex(
                    vertexBuffer[face[0][2]].first,
                    get_diffuse((object.rotationMatrix*object.model->vertices[face[0][2]]) + object.position, object.rotationMatrix*object.model->normals[face[1][2]], camera.position)
                );
                triangleBuffer.push_back(std::pair<std::array<sf::Vertex, 3>, float>{
                    triangle,
                    vertexBuffer[face[0][0]].second + vertexBuffer[face[0][1]].second + vertexBuffer[face[0][2]].second
                });
            }
        }
        delete vertexBuffer;
    }

    std::sort<std::vector<std::pair<std::array<sf::Vertex, 3>, float>>::iterator>(triangleBuffer.begin(), triangleBuffer.end(), compare_depth);

    for (std::pair<std::array<sf::Vertex, 3>, float>& triangle_ : triangleBuffer)
    {
        window.draw((sf::Vertex[3]){triangle_.first[0], triangle_.first[1], triangle_.first[2]}, 3, sf::Triangles);
    }
    

    // Display the result to the screen
    window.display();
}

int main(int argc, char const *argv[])
{
    // Linked model data
    extern const char _binary_obj_cow_obj_start[], _binary_obj_cow_obj_size[];
    const char* binDataStart = _binary_obj_cow_obj_start;
    size_t binDataSize = (size_t)_binary_obj_cow_obj_size;

    Scene scene;

    // Extract model info
    // Model cow = get_model_info(binDataStart, binDataSize);
    // scene.add_model(cow, "cow");
    // scene.spawn_object("cow", "cow");

    // Model teapot = get_model_info_file("C:\\Users\\Nathan\\Documents\\GitHub Repositories\\stuff\\obj_render\\obj\\cow.obj", DO_WHATEVER);
    // scene.add_model(teapot, "teapot");
    // scene.spawn_object("teapot", "teapot");
    
    // Setup the window
    sf::RenderWindow window(sf::VideoMode(WIN_WIDTH, WIN_HEIGHT), "Model rendering", sf::Style::Close | sf::Style::Titlebar | sf::Style::Resize);
    window.setVerticalSyncEnabled(true);
    window.setView(sf::View(sf::FloatRect(-1, 1, 2, -2)));

    sf::Clock clock;

    // Set the initial state of the camera
    Camera camera;
    camera.position[0] = 0;
    camera.position[1] = 0.5;
    camera.position[2] = -5;
    camera.angle[0] = 0;
    camera.angle[1] = 0;
    camera.fovx = 0.79;
    camera.fovy = (WIN_HEIGHT/WIN_WIDTH) * camera.fovx;

    // Required info for mouse drag
    bool isMousePressed(false);
    sf::Vector2i mousePos(0,0);
    
    // Update-draw loop
    while (Update(window, camera, clock, isMousePressed, mousePos, &scene))
    {
        Render(window, &scene, camera);
    }
    
    return 0;
}
