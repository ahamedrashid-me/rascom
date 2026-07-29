module rasbox.main;

import std.stdio;
import std.file;
import std.path;
import std.process;
import std.string;
import std.algorithm;
import std.array;
import std.conv;
import std.datetime.systime;

// --- Config ---
struct Developer {
    string user = "anonymous";
    string email = "unknown";
    string copyright = "Unspecified";
    string signingKey = "~/.rasbox/identity.pem";
}

struct ProjectConfig {
    string name = "ras_app";
    string version_ = "0.1.0";
    string compiler = "rascom";
    string remoteType = "git";
    string remoteUrl = "";
    Developer dev;
}

void main(string[] args) {
    if (args.length < 2) {
        runBuild();
        return;
    }

    string cmd = args[1];

    if (cmd == "-B" || cmd == "--build" || cmd == "build") {
        runBuild();
        return;
    }

    switch (cmd) {
        case "init":
            initProject(".");
            break;

        case "new":
            if (args.length < 3) {
                writeln("rasbox error: usage 'rasbox new <project_name>'");
                return;
            }
            initProject(args[2]);
            break;

        case "clean":
            cleanBuild();
            break;

        case "grab":
            if (args.length < 3) {
                writeln("rasbox error: usage 'rasbox grab <spec|url|file.rcp>'");
                return;
            }
            grabPackage(args[2]);
            break;

        case "install":
            if (args.length < 3) {
                writeln("rasbox error: usage 'rasbox install <file.rcp>'");
                return;
            }
            installRcp(args[2]);
            break;

        case "pull":
            vcsPull();
            break;

        case "push":
            string msg = (args.length >= 3) ? args[2] : "Update Rascode project";
            vcsPush(msg);
            break;

        case "run":
            runBuild();
            string app = buildPath("build", "app");
            if (exists(app)) {
                writeln("rasbox: running ", app, " ...");
                spawnProcess([app]).wait();
            } else {
                writeln("rasbox error: ", app, " not found after build");
            }
            break;

        default:
            if (cmd.startsWith("usr:")) {
                setConfigValue("user", cmd[4 .. $]);
            } else if (cmd.startsWith("mail:")) {
                setConfigValue("email", cmd[5 .. $]);
            } else if (cmd == "-h" || cmd == "--help" || cmd == "help") {
                printHelp();
            } else if (cmd == "-v" || cmd == "--version" || cmd == "version") {
                writeln("rasbox 0.1.0 — RasCode project tool");
            } else {
                writeln("rasbox error: unknown command '", cmd, "'");
                printHelp();
            }
            break;
    }
}

void printHelp() {
    writeln("rasbox — build, packages, and VCS for RasCode");
    writeln();
    writeln("Usage: rasbox [command]");
    writeln();
    writeln("Build:");
    writeln("  (none) | build | -B     Build src/ → build/app via rascom");
    writeln("  run                     Build then run build/app");
    writeln("  clean                   Remove build/");
    writeln();
    writeln("Project:");
    writeln("  init                    Scaffold current directory");
    writeln("  new <name>              Create new project directory");
    writeln();
    writeln("Packages (.rcp archive):");
    writeln("  grab <spec|url|file>    Fetch and install a package");
    writeln("  install <file.rcp>      Install a local .rcp archive");
    writeln();
    writeln("VCS:");
    writeln("  pull                    git pull");
    writeln("  push [msg]              compile-gate + signed commit + push");
    writeln();
    writeln("Identity:");
    writeln("  usr:<name>              Set developer.user in rasbox.toml");
    writeln("  mail:<email>            Set developer.email in rasbox.toml");
}

// ===================================================================
// Config
// ===================================================================

void initProject(string dir) {
    if (dir != "." && !exists(dir)) {
        mkdirRecurse(dir);
    }
    string root = dir == "." ? getcwd() : dir;
    chdir(root);

    if (!exists("src")) mkdir("src");
    if (!exists("lib")) mkdir("lib");
    if (!exists("packages")) mkdir("packages");
    if (!exists("build")) mkdir("build");

    if (!exists("src/main.rco")) {
        std.file.write("src/main.rco",
            "pkg: main;\n\nfnc main[] :: int {\n    show[\"Hello from Rascode!\"];\n    get[0];\n}\n");
    }

    if (!exists("rasbox.toml")) {
        std.file.write("rasbox.toml",
`[package]
name = "ras_app"
version = "0.1.0"
compiler = "rascom"

[developer]
user = "anonymous"
email = "unknown"
copyright = "Unspecified"

[remote]
type = "git"
url = ""

[dependencies]
`);
    }

    if (!exists("README.md")) {
        std.file.write("README.md", "# RasCode project\n\nBuilt with RasBox + rascom.\n");
    }

    writeln("rasbox: project initialized in ", root);
}

ProjectConfig parseConfig() {
    ProjectConfig cfg;
    if (!exists("rasbox.toml")) return cfg;

    string section;
    foreach (line; File("rasbox.toml").byLineCopy()) {
        line = line.strip;
        if (line.length == 0 || line.startsWith("#") || line.startsWith(";")) continue;
        if (line.startsWith("[") && line.endsWith("]")) {
            section = line[1 .. $ - 1].strip;
            continue;
        }
        auto eq = line.indexOf('=');
        if (eq < 0) continue;
        string key = line[0 .. eq].strip;
        string val = line[eq + 1 .. $].strip;
        if (val.startsWith("\"") && val.endsWith("\"")) val = val[1 .. $ - 1];

        if (section == "package") {
            if (key == "name") cfg.name = val;
            else if (key == "version") cfg.version_ = val;
            else if (key == "compiler") cfg.compiler = val;
        } else if (section == "developer") {
            if (key == "user") cfg.dev.user = val;
            else if (key == "email") cfg.dev.email = val;
            else if (key == "copyright") cfg.dev.copyright = val;
            else if (key == "signingKey") cfg.dev.signingKey = val;
        } else if (section == "remote") {
            if (key == "type") cfg.remoteType = val;
            else if (key == "url") cfg.remoteUrl = val;
        }
    }
    return cfg;
}

void setConfigValue(string which, string value) {
    if (!exists("rasbox.toml")) initProject(".");
    string content = readText("rasbox.toml");
    string key = (which == "user") ? "user" : "email";
    // crude in-place replace of key = ...
    import std.regex;
    auto re = regex(`(?m)^` ~ key ~ `\s*=\s*.*$`);
    if (matchFirst(content, re)) {
        content = replaceFirst(content, re, key ~ ` = "` ~ value ~ `"`);
    } else {
        // append under [developer]
        content ~= format("\n%s = \"%s\"\n", key, value);
    }
    std.file.write("rasbox.toml", content);
    writeln("rasbox: set developer.", key, " = ", value);
}

void cleanBuild() {
    if (exists("build")) {
        rmdirRecurse("build");
        writeln("rasbox: cleaned build/");
    } else {
        writeln("rasbox: nothing to clean");
    }
}

// ===================================================================
// Build (rascom has -c and full link; system runtime under /usr/lib/rascom/runtime)
// ===================================================================

void runBuild() {
    ProjectConfig cfg = parseConfig();
    writeln("rasbox: building project '", cfg.name, "' via ", cfg.compiler, "...");

    auto which = execute(["which", cfg.compiler]);
    if (which.status != 0 && !exists(cfg.compiler)) {
        writeln("rasbox error: compiler '", cfg.compiler, "' not found.");
        writeln("  Install rascom or set compiler path in rasbox.toml [package]");
        return;
    }

    if (!exists("build")) mkdirRecurse("build");

    string[] sources;
    if (exists("src")) {
        foreach (DirEntry e; dirEntries("src", SpanMode.shallow)) {
            if (e.name.endsWith(".rco") || e.name.endsWith(".ras"))
                sources ~= e.name;
        }
    }
    if (sources.length == 0) {
        writeln("rasbox: no .rco/.ras sources in src/");
        return;
    }

    // Prefer main entry
    string mainSrc = sources[0];
    foreach (s; sources) {
        string bn = baseName(s).toLower;
        if (bn == "main.rco" || bn == "main.ras") {
            mainSrc = s;
            break;
        }
    }

    // Extra modules: compile to .o with -c (optional multi-file prep)
    string[] extras;
    foreach (s; sources) {
        if (s != mainSrc) extras ~= s;
    }
    foreach (src; extras) {
        string obj = buildPath("build", baseName(src).setExtension("o"));
        string[] ccmd = [cfg.compiler, src, "-c", "-o", obj, "-O2"];
        writeln("rasbox: ", ccmd.join(" "));
        auto cpid = spawnProcess(ccmd);
        if (wait(cpid) != 0) {
            writeln("rasbox: compile-only failed for ", src);
            return;
        }
        writeln("  [✓] ", obj);
    }

    // Main → executable
    string finalApp = buildPath("build", "app");
    string[] cmd = [cfg.compiler, mainSrc, "-o", finalApp, "-O2"];
    writeln("rasbox: ", cmd.join(" "));
    auto pid = spawnProcess(cmd);
    int result = wait(pid);
    if (result != 0) {
        writeln("rasbox: compilation failed for ", mainSrc, " (exit ", result, ")");
        return;
    }
    writeln("  [✓] ", finalApp);
    writeln("rasbox: build complete.");
}

// ===================================================================
// Packages — .rcp is an archive (tar.xz), not a binary
// ===================================================================

/** Install a local .rcp file into packages/ (user) or system if root. */
void installRcp(string rcpPath) {
    if (!exists(rcpPath)) {
        writeln("rasbox error: file not found: ", rcpPath);
        return;
    }
    if (!rcpPath.endsWith(".rcp") && !rcpPath.endsWith(".tar.xz") && !rcpPath.endsWith(".tar.gz")) {
        writeln("rasbox warning: expected .rcp (tar archive); trying anyway...");
    }

    string pkgRoot = exists("/usr/lib/rascom/packages") && isWritable("/usr/lib/rascom/packages")
        ? "/usr/lib/rascom/packages"
        : expandTilde("~/.rascom/packages");
    if (!exists(pkgRoot)) mkdirRecurse(pkgRoot);

    string staging = buildPath(tempDir(), "rasbox_rcp_" ~ to!string(Clock.currTime.toUnixTime));
    mkdirRecurse(staging);
    scope(exit) {
        if (exists(staging)) rmdirRecurse(staging);
    }

    // Extract archive (rcp == tar.xz or plain tar)
    int st;
    if (rcpPath.endsWith(".gz")) {
        st = spawnProcess(["tar", "-xzf", rcpPath, "-C", staging]).wait();
    } else {
        // try xz first, then plain tar
        st = spawnProcess(["tar", "-xJf", rcpPath, "-C", staging]).wait();
        if (st != 0)
            st = spawnProcess(["tar", "-xf", rcpPath, "-C", staging]).wait();
    }
    if (st != 0) {
        writeln("rasbox error: failed to extract ", rcpPath);
        return;
    }

    // Find package.toml for name
    string name = baseName(rcpPath).stripExtension.stripExtension; // file.rcp / file.tar.xz
    string meta = buildPath(staging, "package.toml");
    if (!exists(meta)) {
        // maybe nested single dir
        foreach (DirEntry e; dirEntries(staging, SpanMode.shallow)) {
            if (e.isDir) {
                string m2 = buildPath(e.name, "package.toml");
                if (exists(m2)) {
                    meta = m2;
                    // install from that subdir
                    string dest = buildPath(pkgRoot, baseName(e.name));
                    if (exists(dest)) rmdirRecurse(dest);
                    rename(e.name, dest);
                    writeln("rasbox: installed package → ", dest);
                    recordDependency(baseName(e.name));
                    return;
                }
            }
        }
    }

    // Flat archive: copy staging → packages/<name>
    if (exists(meta)) {
        foreach (line; File(meta).byLineCopy()) {
            auto l = line.strip;
            if (l.startsWith("name")) {
                auto eq = l.indexOf('=');
                if (eq > 0) {
                    name = l[eq + 1 .. $].strip.strip("\"");
                }
            }
        }
    }

    string dest = buildPath(pkgRoot, name);
    if (exists(dest)) rmdirRecurse(dest);
    mkdirRecurse(dest);
    foreach (DirEntry e; dirEntries(staging, SpanMode.shallow)) {
        string target = buildPath(dest, baseName(e.name));
        if (e.isDir) {
            // copy tree
            spawnProcess(["cp", "-a", e.name, target]).wait();
        } else {
            copy(e.name, target);
        }
    }
    writeln("rasbox: installed package '", name, "' → ", dest);
    recordDependency(name);
}

bool isWritable(string path) {
    try {
        string probe = buildPath(path, ".rasbox_write_test");
        std.file.write(probe, "x");
        remove(probe);
        return true;
    } catch (Exception) {
        return false;
    }
}

void recordDependency(string name) {
    if (!exists("rasbox.toml")) return;
    string content = readText("rasbox.toml");
    if (!content.canFind(`"` ~ name ~ `"`)) {
        if (!content.canFind("[dependencies]"))
            content ~= "\n[dependencies]\n";
        content ~= format("\"%s\" = \"latest\"\n", name);
        std.file.write("rasbox.toml", content);
    }
}

void grabPackage(string pkgSpec) {
    writeln("rasbox: grab '", pkgSpec, "'...");

    // Local .rcp file
    if (exists(pkgSpec) && (pkgSpec.endsWith(".rcp") || pkgSpec.endsWith(".tar.xz"))) {
        installRcp(pkgSpec);
        return;
    }

    // Git URL → shallow clone into ./lib
    if (pkgSpec.canFind("://") || pkgSpec.endsWith(".git")) {
        if (!exists("lib")) mkdir("lib");
        string dest = buildPath("lib", baseName(pkgSpec).stripExtension);
        if (exists(dest)) rmdirRecurse(dest);
        auto pid = spawnProcess(["git", "clone", "--depth", "1", pkgSpec, dest]);
        if (wait(pid) != 0) {
            writeln("rasbox error: git clone failed");
            return;
        }
        writeln("rasbox: cloned → ", dest);
        recordDependency(baseName(dest));
        return;
    }

    // Named package: look for local cache / future registry
    string[] search = [
        expandTilde("~/.rascom/packages/" ~ pkgSpec),
        "/usr/lib/rascom/packages/" ~ pkgSpec,
        "/usr/local/lib/rascom/packages/" ~ pkgSpec,
        buildPath("packages", pkgSpec),
        buildPath("lib", pkgSpec),
    ];
    foreach (p; search) {
        if (exists(p)) {
            writeln("rasbox: found installed package at ", p);
            recordDependency(pkgSpec);
            return;
        }
    }

    writeln("rasbox error: package '", pkgSpec, "' not found.");
    writeln("  Try:  rasbox install ./my-pkg-1.0.0.rcp");
    writeln("    or: rasbox grab https://github.com/user/pkg.git");
}

// ===================================================================
// VCS
// ===================================================================

void vcsPull() {
    writeln("rasbox vcs: pulling...");
    auto pid = spawnProcess(["git", "pull"]);
    writeln(wait(pid) == 0 ? "rasbox vcs: pull complete." : "rasbox vcs error: pull failed.");
}

void vcsPush(string commitMsg) {
    ProjectConfig cfg = parseConfig();

    if (cfg.dev.user == "anonymous" || cfg.dev.user.length == 0) {
        writeln("rasbox security error: developer.user not set.");
        writeln("  Run:  rasbox usr:YourName");
        writeln("  Then: rasbox mail:you@example.com");
        return;
    }

    writeln("rasbox security: pre-push compile check...");
    if (!exists("build")) mkdirRecurse("build");
    string mainSrc = "src/main.rco";
    if (!exists(mainSrc)) {
        writeln("rasbox security error: src/main.rco not found.");
        return;
    }
    // Use -c for a fast compile-only gate (no link / runtime needed)
    string checkObj = buildPath("build", "prepush.o");
    auto checkPid = spawnProcess([cfg.compiler, mainSrc, "-c", "-o", checkObj, "-O0"]);
    if (wait(checkPid) != 0) {
        writeln("rasbox security error: Push rejected — source does not compile.");
        return;
    }
    if (exists(checkObj)) remove(checkObj);

    writeln("rasbox vcs: commit as [", cfg.dev.user, "]...");
    spawnProcess(["git", "add", "."]).wait();
    string signedMsg = format("[signed by %s <%s>] %s", cfg.dev.user, cfg.dev.email, commitMsg);
    auto cpid = spawnProcess(["git", "commit", "-m", signedMsg]);
    if (wait(cpid) != 0)
        writeln("rasbox vcs: note — commit may have failed (nothing to commit?)");

    auto pushPid = spawnProcess(["git", "push"]);
    writeln(wait(pushPid) == 0 ? "rasbox vcs: pushed." : "rasbox vcs error: push failed.");
}
