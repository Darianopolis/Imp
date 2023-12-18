if Project "imp" then
    Compile "src/**"
    Include "src"
    Import {
        "glm",
        "ankerl-maps",
        "fmt",

        "fastgltf",
        "stb_image",
        "bc7enc",
    }
end

if Project "imp-cli" then
    Import "imp"
    Compile "cli/**"
    Include "cli"
    Artifact { "out/imp", type = "Console" }
end