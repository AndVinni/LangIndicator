# Gemini CLI Customization

This file allows you to customize the behavior of the Gemini CLI for this project.

## Project Context
Describe your project here. What does it do? What technologies does it use?
Example:
This is a C++ project that implements a language indicator for a specific application. It uses Visual Studio for development.

## Coding Conventions
Specify any coding conventions or style guidelines I should follow.
Example:
- Use snake_case for variable names.
- Use PascalCase for class names.
- Add comments for complex logic.

## Important Files and Directories
List any files or directories that are particularly important or that I should pay special attention to.
Example:
- `main.cpp`: Main entry point of the application.
- `config.h`, `config.cpp`: Configuration files.
- `LangIndicator.h`, `LangIndicator.cpp`: Core logic.

## Gemini Specific Instructions
Add any specific instructions for the Gemini CLI here.
Example:
- When asked to add a new feature, always consider adding a corresponding unit test.
- Before making significant changes, always ask for confirmation.
- Prefer C++17 features where applicable.

- **Caret Position Logic Update:** The logic for displaying the keyboard layout indicator when switching layouts has been updated. The indicator now appears accurately near the editor's caret position. If the caret position cannot be determined (returns 0,0), it defaults to the center of the screen. This involved refining the `FindCaretPosition` function in `LangIndicator.cpp` to use `MapWindowPoints` for more reliable coordinate conversion and correcting DPI scaling issues.

