#include "engine.h"

#define MAX_ARRAY_LENGTH 4096
#define INITIAL_ASTEROID_COUNT 3
#define MAX_LIVES 5
#define POINTS_PER_LARGE_ASTEROID 20
#define POINTS_PER_MEDIUM_ASTEROID 50
#define POINTS_PER_SMALL_ASTEROID 100
#define POINTS_PER_MEDIUM_SAUCER 200
#define POINTS_PER_SMALL_SAUCER 1000
#define POINTS_TO_EXTRA_LIFE 2000

// Types

enum { BACKGROUND, PLAYER, BULLET, ASTEROID, SAUCER };
enum { NONE, SMALL, MEDIUM, LARGE };

typedef struct {
    v3 Position;
    v3 Velocity;
    v3 Scale;
    color Color;
    float Speed;
    float ShootingSpeed; 
    float ProximityLaserDistance;
    float ProximityLaserSpeed;
    float Rotation;
    float MaxVelocity;
    float Accuracy;
    int Lives;
    int Mesh;
    int Texture;
    int Shader;
    int ConstantBuffer;
    int InputLayout;
    int PrimitiveTopology;
    int Lifetime;
    int MaxLifetime;
    int Type;
    int Size;
    int Deleted;
    // ms
    double ShootingDelay;
    double ShootingTimer;
    double ProximityLaserDelay;
    double ProximityLaserTimer;
    double DirectionChangeDelay;
    double DirectionChangeTimer;
    double DeletedDelay;
    double DeletedTimer;
} entity;

typedef struct {
    entity* Items;
    int Length;
    int Capacity;
    int Index;
} entityArray;

typedef struct {
    rectangle Rectangle;
    color Color;
    v3 Scale;
    v3 Position;
} boundingBox;

// Globals

int Pause;
int TestingMode = 1;
int DrawBoundingBoxes;
int MeshAsteroid;
int AsteroidCount;
int ExtraLifeCounter;

u32 Score;

entity Player;
entity Saucer;
entity Background;
entityArray Bullets;
entityArray Asteroids;
entityArray HealthBar;

// colors

color ColorBackground =  {0.2f, 0.2f, 0.2f, 1.0f};
color ColorAsteroid =    {0.3f, 0.3f, 0.3f, 1.0f};
color ColorPlayer =      {1.0f, 0.6f, 0.0f, 1.0f};
color ColorSaucer =      {0.3f, 0.6f, 0.0f, 1.0f};
color ColorBullet =      {1.0f, 0.6f, 1.0f, 1.0f};
color ColorBoundingBox = {0.5f, 0.5f, 0.5f, 1.0f};
color ColorText =        {0.7f, 0.7f, 0.7f, 1.0f};

// Declarations

v3 GetRandomPosition();
v3 GetRandomPositionDistance(entity* Entity, float Distance);
void SpawnSaucer();
v3 GetScaleBySize(int Size);
void CreateBullet(v3 Origin, v3 Direction, float Speed, color Color, int MaxLifetime, int Type);

boundingBox GetEntityBoundingBox(entity* Entity);

entityArray NewEntityArray(int Capacity);
void AddEntityToArray(entityArray* Array, entity* Entity);

void DrawEntityBoundingBox(entity* Entity);
void DrawEntity(entity* Entity);
void DrawEntityArray(entityArray* Array);

void SpawnAsteroid(v3* PositionCenter, int Size);
void HandleOutOfBounds(entity* Entity);

entity* EntityHitsAsteroid(entity* Entity);
void MoveEntity(entity* Entity);
void AccelerateEntity(entity* Entity, v3 Acceleration, float SpeedMultiplier);

// Functions

void AddToScore(int Type, int Size) {
    
    float P = 0;
    
    switch(Type) {
        case ASTEROID: {
            switch(Size) {
                case SMALL: {
                    P = POINTS_PER_SMALL_ASTEROID;
                } break;
                case MEDIUM: {
                    P = POINTS_PER_MEDIUM_ASTEROID;
                } break;
                case LARGE: {
                    P = POINTS_PER_LARGE_ASTEROID;
                } break;
            }
        } break;
        case SAUCER: {
            switch(Size) {
                case MEDIUM: {
                    P = POINTS_PER_SMALL_SAUCER;
                } break;
                case LARGE: {
                    P = POINTS_PER_MEDIUM_SAUCER;
                } break;
            }
        } break;
    }
    
    Score += P;
    ExtraLifeCounter += P;
    if(Player.Lives < MAX_LIVES && ExtraLifeCounter >= POINTS_TO_EXTRA_LIFE) {
        ExtraLifeCounter = 0;
        HealthBar.Items[Player.Lives++].Deleted = 0;
    }
}

v3 GetInaccurateDirection(v3 Direction, float Accuracy) {
    
    v3 NewDirection = {0};
    
    float XOffset = 1.0f - Accuracy; // e.g. 0.1
    XOffset = rand() % (int)(XOffset * 1000.0f); // e.g 0-99
    XOffset /= 1000.0f; // e.g 0.099
    NewDirection.X += Direction.X * XOffset * 2.0f; 
    
    float YOffset = 1.0f - Accuracy;
    YOffset = rand() % (int)(YOffset * 1000.0f);
    YOffset /= 1000.0f; 
    NewDirection.Y += Direction.Y * YOffset * 2.0f; 
    
    V3Normalize(&NewDirection);
    
    return NewDirection;
}

void SpawnAsteroid(v3* PositionCenter, int Size) {
    
    v3 Scale = GetScaleBySize(Size);
    v3 Direction = V3GetRandomV2Direction();
    v3 Position = {0};
    
    if(PositionCenter) {
        Position = V3Add(*PositionCenter, Direction); 
    } else {
        Position = GetRandomPositionDistance(&Player, 5.0f);
    }
    
    entity Asteroid = {
        .Mesh = MeshAsteroid,
        .Shader = DEFAULT_SHADER_POSITION,
        .InputLayout = DEFAULT_INPUT_LAYOUT_POSITION,
        .PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
        .Color = ColorAsteroid,
        .Rotation = (float)(rand() % 361),
        .Scale = Scale,
        .Position = Position,
        .Speed = (float)(rand() % 3 + 1),
        .Velocity = Direction,
        .Type = ASTEROID,
        .Size = Size
    };
    
    AddEntityToArray(&Asteroids, &Asteroid);
    
    ++AsteroidCount;
}

void HandleOutOfBounds(entity* Entity) {
    float Half = Background.Scale.X / 2.0f;
    
    if((Entity->Position.Y >= Half + 1.0f) && Entity->Velocity.Y > 0.0f) {
        Entity->Position.Y = -Half;
    } else if((Entity->Position.Y < -Half - 1.0f) && Entity->Velocity.Y < 0.0f) {
        Entity->Position.Y = Half;
    }
    
    if((Entity->Position.X >= Half + 1.0f) && Entity->Velocity.X > 0.0f) {
        Entity->Position.X = -Half;
    } else if((Entity->Position.X < -Half - 1.0f) && Entity->Velocity.X < 0.0f) {
        Entity->Position.X = Half;
    }
}

entity* EntityHitsAsteroid(entity* Entity) {
    for(int Index = 0; Index < Asteroids.Length; ++Index) {
        entity* Asteroid = &Asteroids.Items[Index];
        
        if(Asteroid->Deleted || Entity->Deleted) continue;
        
        boundingBox EntityBox = GetEntityBoundingBox(Entity);
        boundingBox AsteroidBox = GetEntityBoundingBox(Asteroid);
        
        if(RectanglesIntersect(EntityBox.Rectangle, AsteroidBox.Rectangle)) {
            return Asteroid;
        }
    }
    return NULL;
}

void RotateEntity(entity* Entity, float Degrees) {
    Entity->Rotation += Degrees;
    if(Entity->Rotation >= 360.0f) Entity->Rotation = 0.0f;
}

void MoveEntity(entity* Entity) {
    
    // position += velocity * dt * speed
    
    if(Entity->Deleted) return;
    
    Entity->Position = V3Add(Entity->Position, V3MultiplyScalar(Entity->Velocity, DeltaTime * Entity->Speed));
    
    HandleOutOfBounds(Entity);
}

void AccelerateEntity(entity* Entity, v3 Acceleration, float SpeedMultiplier) {
    
    // velocity += acceleration * dt * speed * (multiplier)
    
    Entity->Velocity = V3Add(Entity->Velocity, 
                             V3MultiplyScalar(Acceleration, DeltaTime * Entity->Speed * SpeedMultiplier));
}

entityArray NewEntityArray(int Capacity) {
    entityArray Array = {
        .Items = MemoryAlloc(Capacity * sizeof(entity)),
        .Capacity = Capacity,
    };
    return Array;
}

// Overrides elements starting from index 0 if overflowing

void AddEntityToArray(entityArray* Array, entity* Entity) {
    Array->Items[Array->Index++] = *Entity;
    if(Array->Length < Array->Capacity) {
        ++Array->Length;
    }
    if(Array->Index >= Array->Capacity) {
        Array->Index = 0;
    }
}

boundingBox GetEntityBoundingBox(entity* Entity) {
    
    matrix Transform = MatrixIdentity();
    matrix Rotation = MatrixRotationZ(Entity->Rotation);
    matrix Scale = MatrixScale(Entity->Scale); 
    
    Transform = MatrixMultiply(&Transform, &Scale);
    Transform = MatrixMultiply(&Transform, &Rotation);
    
    mesh* Mesh = &Meshes[Entity->Mesh];
    int StrideInt = Mesh->Stride / sizeof(float);
    
    // Transform vertices and get the bounding rectangle 
    
    rectangle Rectangle = {0};
    
    for(int Index = 0; Index < Mesh->NumVertices;
        ++Index) {
        v3 Vertex = {
            Mesh->Vertices[Index * StrideInt],
            Mesh->Vertices[Index * StrideInt + 1],
            Mesh->Vertices[Index * StrideInt + 2],
        };
        
        Vertex = MatrixV3Multiply(Transform, Vertex);
        
        if(Vertex.X < Rectangle.Left) {
            Rectangle.Left = Vertex.X; 
        }
        if(Vertex.X > Rectangle.Right) {
            Rectangle.Right = Vertex.X; 
        }
        if(Vertex.Y < Rectangle.Bottom) {
            Rectangle.Bottom = Vertex.Y; 
        }
        if(Vertex.Y > Rectangle.Top) {
            Rectangle.Top = Vertex.Y; 
        }
    }
    
    float XScale = Rectangle.Right - Rectangle.Left;
    float YScale = Rectangle.Top - Rectangle.Bottom;
    
    Rectangle.Left = Rectangle.Left + Entity->Position.X; 
    Rectangle.Right = Rectangle.Left + XScale;
    Rectangle.Top = Rectangle.Top + Entity->Position.Y;
    Rectangle.Bottom = Rectangle.Top - YScale;
    
    // We store the Rectangle for intersect tests and
    // Position & Scale for drawing the box
    
    boundingBox BoundingBox = {
        .Rectangle = Rectangle,
        .Scale = {XScale, YScale, 1.0f},
        .Position = {
            Rectangle.Right - XScale / 2.0f, 
            Rectangle.Top - YScale / 2.0f, 
            0.0f
        },
        .Color = ColorBoundingBox,
    };
    
    return BoundingBox;
}

v3 GetScaleBySize(int Size) {
    v3 Scale = {1.0f, 1.0f, 1.0f};
    switch(Size) {
        case SMALL: {
            Scale.X = 0.5f; 
            Scale.Y = 0.5f; 
        } break;
        case LARGE: {
            Scale.X = 2.0f; 
            Scale.Y = 2.0f; 
        } break;
    }
    return Scale;
}

void DrawEntity(entity* Entity) {
    if(Entity->Deleted) return;
    DrawObject(Entity->Position,
               Entity->Scale,
               Entity->Rotation,
               Entity->Color,
               Entity->Mesh,
               Entity->Texture,
               Entity->Shader,
               Entity->ConstantBuffer,
               Entity->InputLayout,
               Entity->PrimitiveTopology);
    if(DrawBoundingBoxes && Entity->Type != BACKGROUND) {
        DrawEntityBoundingBox(Entity);
    }
}

void DrawEntityBoundingBox(entity* Entity) {
    
    boundingBox BoundingBox = GetEntityBoundingBox(Entity);
    
    entity BoundingBoxEntity = (entity){
        .Mesh =              DEFAULT_MESH_RECTANGLE_LINES,
        .Shader =            DEFAULT_SHADER_POSITION,
        .InputLayout =       DEFAULT_INPUT_LAYOUT_POSITION,
        .PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST,
        .Color =             BoundingBox.Color,
        .Scale =             BoundingBox.Scale,
        .Position =          BoundingBox.Position
    };
    
    DrawEntity(&BoundingBoxEntity);
}

v3 GetRandomPosition() {
    return (v3) {
        (float)((rand() % (int)Background.Scale.X) - (Background.Scale.X / 2.0f)),
        (float)((rand() % (int)Background.Scale.Y) - (Background.Scale.Y / 2.0f))
    };
}

void DrawEntityArray(entityArray* Array) {
    for(int Index = 0; Index < Array->Length; ++Index) {
        DrawEntity(&Array->Items[Index]);
    }
}

void SpawnAsteroids(int Count, entity* Entity) {
    if(Entity != NULL) {
        for(int Index = 0; Index < Count; ++Index) {
            SpawnAsteroid(&Entity->Position, Entity->Size-1);
        }
    } else {
        for(int Index = 0; Index < Count; ++Index) {
            SpawnAsteroid(NULL, LARGE);
        }
    }
}

entity* AsteroidNear(entity* Entity, float Range) {
    for(int Index = 0; Index < Asteroids.Length; ++Index) {
        entity* Asteroid = &Asteroids.Items[Index];
        if(Asteroid->Deleted) continue;
        
        float Distance = V3GetDistance(Asteroid->Position, Entity->Position);
        
        if(Distance <= Range) return Asteroid;
    }
    
    return NULL;
}

void ReduceLives(entity* Entity) {
    if(TestingMode) return;
    HealthBar.Items[--Player.Lives].Deleted = 1;
    if(Player.Lives <= 0) Running = 0;
}

void Init() {
    
    ColorSaucer = ColorLightBlue;
    
    // Inits
    
    InitTimer(&Timer);
    EngineColorBackground = (color){0.12f, 0.12f, 0.12f, 1.0f};
    
    Bullets = NewEntityArray(100);
    Asteroids = NewEntityArray(100);
    
    // Background
    
    Background = (entity){
        .Mesh = DEFAULT_MESH_RECTANGLE,
        .Shader = DEFAULT_SHADER_POSITION,
        .InputLayout = DEFAULT_INPUT_LAYOUT_POSITION,
        .PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
        .Color = ColorBackground,
        .Scale = {15.0f, 15.0f, 1.0f},
    };
    
    // Healthbar
    
    HealthBar = NewEntityArray(5);
    
    float XOffset = 0.7f;
    float YOffset = -1.0f;
    
    color Color = ColorLightBlue;
    
    for(int Index = 0; Index < MAX_LIVES; ++Index) {
        entity Entity = {
            .Mesh = DEFAULT_MESH_RECTANGLE,
            .Shader = DEFAULT_SHADER_POSITION,
            .InputLayout = DEFAULT_INPUT_LAYOUT_POSITION,
            .PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
            .Color = Color,
            .Scale = {0.5f, 0.5f, 1.0f},
            .Position = {
                Background.Scale.X / 2.0f - XOffset, 
                Background.Scale.Y / 2.0f + YOffset , 
                0.0f},
        };
        
        XOffset += Entity.Scale.X * 1.1f;
        Color.R -= 0.20f;
        Color.B += 0.1f;
        AddEntityToArray(&HealthBar, &Entity); 
    }
    
    // Player
    
    int PlayerTexture = CreateTexture("player.png", NULL, 0);
    
    Player = (entity){
        .Mesh = DEFAULT_MESH_RECTANGLE_UV,
        .Shader = DEFAULT_SHADER_POSITION_UV,
        .InputLayout = DEFAULT_INPUT_LAYOUT_POSITION_UV,
        .PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
        .Texture = PlayerTexture,
        .Color = ColorLightBlue,
        .Speed = 3.0f,
        .Scale = {1.0f, 1.0f, 1.0f},
        .MaxVelocity = 4.0f,
        .Type = PLAYER,
        .Lives = MAX_LIVES
    };
    
    // Flying saucer
    
    int SaucerTexture = CreateTexture("saucer.png", NULL, 0);
    
    Saucer = (entity){
        .Mesh = DEFAULT_MESH_RECTANGLE_UV,
        .Shader = DEFAULT_SHADER_POSITION_UV,
        .InputLayout = DEFAULT_INPUT_LAYOUT_POSITION_UV,
        .PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
        .Texture = SaucerTexture,
        .Color = ColorOrangeTomato,
        .Speed = 3.0f,
        .Scale = {1.0f, 0.5f, 1.0f},
        .Velocity = {1.0f, 0.0f, 0.0f},
        .MaxVelocity = 4.0f,
        .Accuracy = 0.1f,
        .Type = SAUCER,
        .Size = MEDIUM,
        .DirectionChangeDelay = 4000.0f,
        .DirectionChangeTimer = Timer.ElapsedMilliSeconds,
        .ShootingDelay = 2000.0f,
        .ShootingTimer = Timer.ElapsedMilliSeconds,
        .ShootingSpeed = 3.0f,
        .ProximityLaserSpeed = 15.0f,
        .ProximityLaserDelay = 100.0f,
        .ProximityLaserTimer = Timer.ElapsedMilliSeconds,
        .ProximityLaserDistance = 3.0f,
        .DeletedDelay = 5000.0f,
        .DeletedTimer = Timer.ElapsedMilliSeconds,
        .Deleted = 1,
    };
    
    // Asteroids
    
    float AsteroidVertexData[] = {
        -0.5f, 0.0f, 0.0f,
        -0.2f, 0.5f, 0.0f,
        0.5f, 0.5f, 0.0f,
        
        -0.5f, 0.0f, 0.0f,
        0.5f,  0.5f, 0.0f,
        0.1f, -0.5f, 0.0f,
        
        0.1f, -0.5f, 0.0f,
        0.5f,  0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
    };
    
    MeshAsteroid = CreateMesh(AsteroidVertexData, sizeof(AsteroidVertexData),
                              3, 0, 0);
    
    SpawnAsteroids(INITIAL_ASTEROID_COUNT, NULL);
}

void IncreaseDifficulty() {
    if(Score > 5000.0f) {
        Saucer.Accuracy = 0.8f;
    } else if(Score > 10000.0f) {
        Saucer.Accuracy = 1.0f;
    }
}

void Input() {
    
    // Inputs
    
    if(KeyPressed[P]) {
        Pause = (Pause) ? 0 : 1;
        KeyPressed[P] = 0;
    }
    
    if(KeyPressed[B]) {
        DrawBoundingBoxes = (DrawBoundingBoxes) ? 0 : 1;
        KeyPressed[B] = 0;
    }
    
    if(KeyPressed[R]) {
        Camera.Position = (v3){0.0f, 0.0f, -14.5f};
        KeyPressed[R] = 0;
    }
    
    if(KeyPressed[T]) {
        TestingMode = (TestingMode) ? 0 : 1;
        KeyPressed[T] = 0;
    }
    
    // Direction
    
    if(KeyDown[LEFT]) RotateEntity(&Player, 5.0f);
    if(KeyDown[RIGHT]) RotateEntity(&Player, -5.0f);
    float Theta = DegreesToRadians(Player.Rotation);
    v3 Direction = {cos(Theta), sin(Theta)};
    
    // Acceleration
    
    v3 Acceleration = {0};
    if(KeyDown[UP]) Acceleration = Direction;
    AccelerateEntity(&Player, Acceleration, 4.0f);
    
    // Shooting
    
    if(KeyPressed[SPACE]) {
        KeyPressed[SPACE] = 0;
        CreateBullet(Player.Position, Direction, 6.0f, ColorBullet, 120, PLAYER);
    }
}

void DeleteEntity(entity* Entity) {
    Entity->Deleted = 1;
    switch(Entity->Type) {
        case SAUCER: {
            Entity->DeletedTimer = Timer.ElapsedMilliSeconds;
        } break;
        case ASTEROID: {
            --AsteroidCount;
        } break;
    }
}

// Get random position that is at least Distance away from Entity
v3 GetRandomPositionDistance(entity* Entity, float Distance) {
    v3 Position = {0};
    do { Position = GetRandomPosition(); } 
    while(V3GetDistance(Entity->Position, Position) < Distance);
    return Position;
}

void SpawnSaucer() {
    Saucer.Velocity = V3GetRandomV2Direction();
    Saucer.Position = GetRandomPositionDistance(&Player, 5.0f);
    Saucer.Deleted = 0;
    Saucer.Size = rand() % 2 + 1;
    Saucer.Scale = GetScaleBySize(Saucer.Size);
    // hack to squeeze the icon
    Saucer.Scale.Y /= 2.0f;
}

void CreateBullet(v3 Origin, v3 Direction, float Speed, color Color, int MaxLifetime, int Type) {
    entity Bullet = {
        .Mesh = DEFAULT_MESH_RECTANGLE,
        .Shader = DEFAULT_SHADER_POSITION,
        .InputLayout = DEFAULT_INPUT_LAYOUT_POSITION,
        .PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
        .Color = Color,
        .Speed = Speed,
        .Velocity = Direction,
        .Scale = {0.1f, 0.1f, 1.0f},
        .Position = V3Add(Origin, Direction),
        .MaxLifetime = MaxLifetime,
        .Type = Type
    };
    
    AddEntityToArray(&Bullets, &Bullet);
}

int EntitiesCollide(entity* A, entity* B) {
    
    if(A->Deleted || B->Deleted) return 0;
    
    boundingBox ABox = GetEntityBoundingBox(A);
    boundingBox BBox = GetEntityBoundingBox(B);
    if(RectanglesIntersect(ABox.Rectangle, BBox.Rectangle)) {
        return 1;
    }
    return 0;
}

void Update() {
    
    if(Pause) return;
    
    // Player
    
    // cap velocity
    
    if(V3Length(&Player.Velocity) > Player.MaxVelocity) {
        V3Normalize(&Player.Velocity);
        Player.Velocity = V3MultiplyScalar(Player.Velocity, Player.MaxVelocity);
    }
    
    // slow down
    
    Player.Velocity = V3MultiplyScalar(Player.Velocity, 0.99f);
    
    // move
    
    MoveEntity(&Player);
    
    // Saucer
    
    if(!Saucer.Deleted) {
        
        // change direction
        
        if(TimeElapsed(&Saucer.DirectionChangeTimer, Saucer.DirectionChangeDelay)) {
            Saucer.Velocity = V3GetRandomV2Direction();
        } 
        
        // shoot player
        
        if(TimeElapsed(&Saucer.ShootingTimer, Saucer.ShootingDelay)) {
            
            v3 Direction = V3GetRandomV2Direction();
            
            if(Saucer.Size == SMALL) {
                Direction = V3GetDirection(Saucer.Position, Player.Position);
                if(Saucer.Accuracy < 1.0f) {
                    Direction = GetInaccurateDirection(Direction, Saucer.Accuracy);
                }
            }
            
            CreateBullet(Saucer.Position,
                         Direction,
                         Saucer.ShootingSpeed,
                         ColorYellow,
                         120,
                         SAUCER);
        }
        
        // shoot asteroids
        
        entity* Asteroid = NULL;
        
        if(Asteroid = AsteroidNear(&Saucer, Saucer.ProximityLaserDistance)) {
            
            if(TimeElapsed(&Saucer.ProximityLaserTimer, Saucer.ProximityLaserDelay)) {
                v3 Direction = V3GetDirection(Saucer.Position, Asteroid->Position);
                CreateBullet(Saucer.Position, Direction, Saucer.ProximityLaserSpeed, ColorOrange, 30, SAUCER);
            }
        }
        
        MoveEntity(&Saucer);
        
        if(Asteroid = EntityHitsAsteroid(&Saucer)) {
            DeleteEntity(Asteroid);
            DeleteEntity(&Saucer);
            if(Asteroid->Size > SMALL) {
                SpawnAsteroids(INITIAL_ASTEROID_COUNT, Asteroid);
            }
        }
        
        if(EntitiesCollide(&Saucer, &Player)) {
            DeleteEntity(&Saucer);
            ReduceLives(&Player);
        }
        
    }
    
    if(Saucer.Deleted && TimeElapsed(&Saucer.DeletedTimer, Saucer.DeletedDelay)) {
        SpawnSaucer();
    }
    
    // Bullets
    
    for(int Index = 0; Index < Bullets.Length; ++Index) {
        entity* Bullet = &Bullets.Items[Index];
        if(Bullet->Deleted) continue;
        if(++Bullet->Lifetime >= Bullet->MaxLifetime) {
            Bullet->Deleted = 1;
        } else {
            MoveEntity(Bullet);
            entity* Asteroid = NULL;
            
            int PlayerCollides = 0;
            
            if(Bullet->Type == PLAYER) {
                if(EntitiesCollide(Bullet, &Saucer)) {
                    DeleteEntity(&Saucer);
                    AddToScore(Saucer.Type, Saucer.Size);
                }
            }
            
            if(Bullet->Type == SAUCER) {
                if(EntitiesCollide(Bullet, &Player)) {
                    DeleteEntity(Bullet);
                    ReduceLives(&Player);
                    PlayerCollides = 1;
                    // TODO: nicer kickback
                    v3 Kickback = Bullet->Velocity;
                    V3Normalize(&Kickback);
                    Kickback = V3MultiplyScalar(Kickback, 0.3f);
                    Player.Position = V3Add(Player.Position, Kickback);
                }
            }
            
            if(Asteroid = EntityHitsAsteroid(Bullet)) {
                DeleteEntity(Bullet);
                DeleteEntity(Asteroid);
                if(Bullet->Type == PLAYER) {
                    AddToScore(Asteroid->Type, Asteroid->Size);
                }
                
                if(Asteroid->Size > SMALL) {
                    SpawnAsteroids(INITIAL_ASTEROID_COUNT, Asteroid);
                }
                
            }
        }
    }
    
    // Asteroids
    
    int PlayerCollides = 0;
    
    for(int Index = 0; Index < Asteroids.Length; ++Index) {
        entity* Asteroid = &Asteroids.Items[Index];
        if(Asteroid->Deleted) continue;
        
        // Rotate & move
        
        RotateEntity(Asteroid, 0.2f);
        MoveEntity(Asteroid);
        
        // Player collision
        
        if(EntitiesCollide(&Player, Asteroid)) {
            PlayerCollides = 1;
            Asteroid->Color = ColorRed;
            DeleteEntity(Asteroid);
            if(Asteroid->Size > SMALL) {
                SpawnAsteroids(INITIAL_ASTEROID_COUNT, Asteroid);
            }
            ReduceLives(&Player);
        } else {
            Asteroid->Color = ColorAsteroid;
        }
    }
    
    // Player.Color = (PlayerCollides ? ColorRed : ColorPlayer);
    
    if(AsteroidCount <= 0) {
        SpawnAsteroids(INITIAL_ASTEROID_COUNT, NULL);
    }
    
    // Difficulty
    
    if(Score > 5000.0f && Score < 20000.0f) {
        IncreaseDifficulty();
    }
    
};

void DrawScore() {
    
    char Text[50] = {0};
    sprintf(Text, "%u", Score); 
    DrawString((v3){
                   -(Background.Scale.X / 2.0f) + 1.0f, 
                   Background.Scale.Y / 2.0f - 1.0f, 
                   0.0f
               },
               Text, 
               ColorText,
               (v3){0.8f, 0.8f, 1.0f}
               );
}

void Draw() {
    DrawEntity(&Background);
    DrawEntity(&Player);
    DrawEntity(&Saucer);
    DrawEntityArray(&Bullets);
    DrawEntityArray(&Asteroids);
    DrawEntityArray(&HealthBar);
    DrawScore();
}

