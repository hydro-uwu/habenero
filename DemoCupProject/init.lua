DemoScene = {}

-- This file is the entry point of the application, it defines the main scene and the main class to use.
-- Init is found by the engine and is used to initialize the application, it should return a table with the following fields:
Init =  {
    -- The main scene to load when the application starts. This should be a glTF file.
    -- optionaly supports glb, obj and or pretty much anything that Raylib can load as a model.
    MainScene = "models/MainScene.gltf",
    -- the main class
    MainClass = DemoScene,
    Debug = true, -- if true, the engine will print debug information to the console
}

-- optionaly include any extra files you need here, they will be included before the main scene is loaded.

function DemoScene:Init()
    -- this is called when the scene is loaded, you can do any initialization here
    print("DemoScene initialized")
    
end

function DemoScene:Update()
    -- this is called every frame, you can do any updates here
end

function DemoScene:Draw()
    -- this is called every frame, you can do any drawing here
end