import shutil
import os
import fnmatch
import subprocess
import sys


def _clean_dir(path):
    dirs_to_remove = {
        'build',
        'dist',
        '*.egg-info',
        '.pytest_cache'
    }

    files_to_remove = {
        '*.so',
        '*.pyd',
        '*.pyc',
        '.coverage'
    }

    # Remove directories
    for item in os.listdir(path):
        if os.path.isdir(item) and any(fnmatch.fnmatch(item, pattern) for pattern in dirs_to_remove):
            shutil.rmtree(item, ignore_errors = True)
            print(f"Removed directory: {item}")
        elif os.path.isfile(item) and any(fnmatch.fnmatch(item, pattern) for pattern in files_to_remove):
            os.remove(item)
            print(f"Removed file: {item}")

    print("Clean completed successfully!")


def clean():
    """
    Cleans up the current directory by removing specified unwanted files and directories.

    This function is typically used for cleanup, such as removing build artifacts,
    cache files, and compiled objects to ensure a clean environment.

    :raises FileNotFoundError: If a specified file or directory does not exist during the removal process.
    :raises PermissionError: If the function lacks the permissions to remove specific files or directories.
    """
    cwd = os.getcwd()
    project = os.path.join(cwd, 'specio3')
    print (project)
    _clean_dir(os.getcwd())
    _clean_dir(project)


def build():
    """
    Builds the project by running the setup script.
    :raises Exception: If the setup script fails to execute properly.
    """
    os.system('python setup.py build_ext --inplace')
    print("Build completed successfully!")


def rebuild():
    """
    Rebuilds the project by cleaning the current directory and then running the setup script.

    This function is typically used to prepare the project for distribution or testing
    by ensuring that all build artifacts are removed before the build process begins.

    :raises Exception: If the setup script fails to execute properly.
    """
    clean()
    os.system('python setup.py build_ext --inplace')
    print("Rebuild completed successfully!")


def coverage():
    """
    Runs the test suite with coverage reporting.
    """
    os.system('python -m pytest --cov=specio3')
    print("Coverage completed successfully!")


def test():
    """
    Runs the test suite without coverage reporting.
    """
    os.system('python -m pytest -s -v')
    print("Tests completed successfully!")


def format():
    """
    Format Python code using Black code formatter.

    This function runs Black on the specio3 package and tests directory
    to ensure consistent code formatting across the project.
    """
    print("Formatting Python code with Black...")
    try:
        subprocess.run([sys.executable, "-m", "black", "specio3/", "tests/", "scripts.py"], check=True)
        print("✅ Code formatting completed successfully!")
    except subprocess.CalledProcessError as e:
        print(f"❌ Code formatting failed: {e}")
        sys.exit(1)


def lint():
    """
    Check Python code formatting with Black (dry-run).

    This function runs Black in check mode to verify that all Python code
    follows the project's formatting standards without making changes.
    """
    print("Checking code formatting with Black...")
    try:
        subprocess.run([sys.executable, "-m", "black", "--check", "--diff", "specio3/", "tests/", "scripts.py"], check=True)
        print("✅ All code is properly formatted!")
    except subprocess.CalledProcessError as e:
        print("❌ Code formatting issues found. Run 'poetry run format' to fix them.")
        sys.exit(1)
