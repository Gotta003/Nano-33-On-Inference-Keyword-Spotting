import sys
import importlib
import re

GREEN = "\033[0;32m"
RED = "\033[0;31m"
RESET = "\033[0m"

NAME_MAP={
    "scikit-learn": "sklearn"
}

def get_requirements(file_path="requirements.txt"):
    packages=[]
    try:
        with open(file_path, "r") as f:
            for line in f:
                line=line.strip()
                if not line or line.startswith("#"):
                    continue
                pkg_name=re.split(r'[<>=!@;]', line)[0].strip()
                if pkg_name:
                    packages.append(pkg_name)
    except FileNotFoundError:
        print(f"{RED}Error: {file_path} not found.{RESET}")
        sys.exit(1)
    return packages

def check_libraries():
    packages=get_requirements()
    failed=[]
    print(f"{'PACKAGE':<20} {'VERSION':<20}")
    print("-"*40)
    for pkg in packages:
        try:
            import_name=NAME_MAP.get(pkg.lower(), pkg)
            module=importlib.import_module(import_name)
            version=getattr(module, "__version__", "Installed (version unknown)")
            print(f"{GREEN}{pkg:<20} {version}{RESET}")
        except ImportError:
            print(f"{RED}{pkg:<20} NOT FOUND{RESET}")
            failed.append(pkg)
        except Exception as e:
            print(f"{RED}{pkg:<20} Error: {str(e)}{RESET}")
            failed.append(pkg)
    if failed:
        print(f"\n{RED}Missing packages: {', '.join(failed)}{RESET}")
        sys.exit(1)
    else:
        print(f"\n{GREEN}All libraries from requirements.txt were verified!{RESET}")
        
if __name__=="__main__":
    check_libraries()