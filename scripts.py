import shutil
import os
import fnmatch


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