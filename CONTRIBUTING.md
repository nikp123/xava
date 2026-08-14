# Coding conventions
* 4 spaces for indents
* curly braces on same line as if, while, for statements

Generally please try to keep the style consistent with the code as it is.

# AI assistance attribution
AI-generated or AI-assisted changes must be disclosed in every commit, whether
written by a human using an AI tool or by a coding agent. Append a git trailer
to the commit message naming the assistant that produced the change:

```
Co-authored-by: <Assistant Name> <email>
```

For example, when the change was written by DeepSeek:

```
Co-authored-by: DeepSeek-V4 Flash <ai@deepseek.com>
```

Name the specific model/assistant; if several tools helped, add one trailer per
tool. See `AGENTS.md` for the version of these rules aimed at coding agents. 
