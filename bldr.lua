if Project "imp" then
    Compile "src/**"
    Include "src"
end

if Project "imp-cli" then
    Import "imp"
    Compile "cli/**"
    Include "cli"
    Artifact { "out/imp", type = "Console" }
end