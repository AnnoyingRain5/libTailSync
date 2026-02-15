import subprocess
import os
import shutil

Import('env')

if "TAILSYNC_ENABLE_WEBCONFIG" in env["CPPDEFINES"]:
    # clear out dir
    if os.path.exists("src/WebConfig/generated"):
        shutil.rmtree("src/WebConfig/generated")
    os.mkdir("src/WebConfig/generated")
    for file in os.listdir("src/WebConfig/html"):
        if file == "readme.md":
            continue
        filepath = os.path.join("src/WebConfig/html", file)
        with open(filepath, "r") as infile, open(f"src/WebConfig/generated/{file}.h", "w") as outfile:
            subprocess.call(["bin2c", file.replace(".", "_")], stdin=infile, stdout=outfile)
