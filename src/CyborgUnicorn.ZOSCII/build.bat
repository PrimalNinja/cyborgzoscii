REM dotnet nuget push bin/NuGet/CyborgUnicorn.ZOSCII.*.nupkg --api-key YOUR_KEY --source https://api.nuget.org/v3/index.json
dotnet pack CyborgUnicorn.ZOSCII.csproj -c Release -o bin/NuGet
