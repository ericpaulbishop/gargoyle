# Contributing to Gargoyle

Firstly, thanks for taking the time to contribute! Help is always appreciated.

The following is a set of guidelines for contributing to Gargoyle. These are mostly guidelines, not rules. Use your best judgment, and feel free to propose changes to this document in a pull request. If you haven't read the README yet, that might be a good place to start.

## Code of Conduct

This project and everyone participating in it is governed by the following brief Code of Conduct. By participating, you are expected to uphold this code.
* Using welcoming and inclusive language
* Being respectful of differing viewpoints and experiences
* Gracefully accepting constructive criticism
* Focusing on what is best for the community
* Showing empathy towards other community members

This is a community project contributed to by generous individuals out of their own time.

## Project Conventions

### Gargoyle specific code syntax

The Linux Kernel coding style document should largely be followed, except for the following (not all inclusive) disagreements:
* Indentation should ALWAYS be done with tabs, not spaces. One tab per indent. After the first indent, offsets from the first character should be spaces, not tabs.
* K&R brace style is evil. Curly braces -- both opening and closing -- get a line to themselves. The ONLY exception is if it is a one-line loop or if statement that has been put on a single line for simplicity.
* One-line if statements/functions/loops should only be used if there is at most one statement enclosed in the brackets.
* All loops and if statements should have curly braces around them, even if they are just one line

#### Package Conventions

Gargoyle has a mix of references to upstream packages as well as packages that are stored locally in this repository. There are various reasons for this, but the following considerations are the main ones you should use when deciding what to do:
* If the upstream package is largely static and unchanging, with infrequent API (particularly ABI) changes, it is usually ok to reference it
  * If the package is not suitable for Gargoyle in its base form and the changes required are minimal, a patch can be used to fix this. Otherwise, consider hosting the package in the repository.
> NOTE: When users install packages, gpkg will try to find the newest version available, which may be different to what is referenced in the Gargoyle build system. If this will break the package, you need to do something.

### Plugin Conventions

* All Gargoyle plugins are to be named `plugin-gargoyle-*`
  * All plugins are to be written compatible with the existing i18n implementation in Gargoyle
  * All plugins are to have a minimum of an English-EN translation
    * Plugin language files are to be stored in the language plugin itself, not alongside the plugin in question
* All language plugins are to be named `plugin-gargoyle-i18n-<Language Name>-<ISO 639-1 Code>`
* If the plugin is creating a GUI for an existing upstream package and the package is to be stored in the Gargoyle repository, the plugin and the package should be stored separately (see minidlna for example)

### Target Conventions

Gargoyle targets (and their configs) are always _generic_, not device specific (don't confuse targets with profiles, these are different). If you find yourself setting individual device config items like `CONFIG_TARGET_ar71xx_generic_DEVICE_archer-c7-v2=y` instead of `CONFIG_TARGET_ar71xx=y`, stop, go back a few steps.

> NOTE: It is perfectly acceptable to end up with a target that only includes a single device, as long as the target is still configured generically so that it is expandable easily in the future.

Please thoroughly test any new targets being submitted. Try getting some of the community to help you test them.

## How Can I Contribute?

### Reporting Bugs

Following these guidelines helps maintainers and the community understand your report, reproduce the behavior, and find related reports.

> **Note:** If you find a **Closed** issue that seems like it is the same thing that you're experiencing, open a new issue and include a link to the original issue in the body of your new one.

#### Before Submitting A Bug Report

* **Try debugging it.** You might be able to find the cause of the problem and fix things yourself. Most importantly, check if you can reproduce the problem in the latest version of Gargoyle (as it may have already been fixed).
* **Check the forums** for any similar reports or problems. It might have already been solved there, or it might not even be a bug.
* **Perform a search of existing issues** to see if the problem has already been reported. If it has **and the issue is still open**, add a comment to the existing issue instead of opening a new one.

#### How Do I Submit A (Good) Bug Report?

Bugs are tracked as GitHub issues. Explain the problem and include additional details to help maintainers reproduce the problem:

* **Use a clear and descriptive title** for the issue to identify the problem.
* **Gargoyle Version** you experienced the bug on, and whether you have tested it on previous/subsequent versions
* **Router Make, Model and Version** so we can determine if it is a device specific bug
* **What were you doing when the bug occurred?** If you find yourself writing the phrase `and then the bug happened`, you probably aren't being descriptive enough.
* **What happened (what is the bug)?** Why do you think that is a bug or unacceptable behaviour?
* **What did you expect to happen instead?** What is your expectation of the proper behaviour?
* **Can you reproduce the bug everytime, sometimes or rarely?** For rare issues, start with the forums and see if others are seeing the same issue.
* **Exact steps to reproduce the bug** Numbered steps, leaving nothing out
* **Pictures, videos, GIFs, evidence!** Pictures say a thousand words. Videos and GIFs say so much more. Show us what you're talking about.

### Suggesting Enhancements

Enhancement suggestions are welcome, however they generally get a lower priority than bug fixes. Additionally, some of them may never happen, or may be so far down the priority list that they need someone to volunteer their time and effort to get them done (maybe that person is **you**).

Please post enhancement suggestions to the forum to gain feedback. Don't worry, we look for things there too.

### Your First Code Contribution

Your first contribution to the project might be a bit scary and daunting. Don't worry, none of the developers bite and are always happy to receive help. In return, you will receive:
* Thorough code review
* Feedback
* Constructive criticism
* Our thanks
  * Sometimes we don't remember to say thank you for every piece of code that gets merged. Please accept it here, THANK YOU! The fact that you submitted something and we went to the effort of merging it means that we are happy and very thankful for your contribution.

For your first contribution, it might be worth dropping us a line on the forums first to check that what you want to do is worthwhile. We don't want you to waste time when you could have contributed to something else that deserves your attention more!

#### Local development

All contributions should be developed locally and tested locally before being submitted. Untested code (no matter how confident you are) likes to bite you when you turn your back. Trust us, we've been there.

### Pull Requests

When you are ready to submit your code, make a Pull Request. Include as much detail as possible. If you're fixing an open issue, make sure you tag the issue in your description. If you're adding a new feature, make sure you document how it works and why we need it. Don't be afraid to add links back to the forum, great discussion happens there as well and adds lots of context.

Make sure you include a descriptive title that is short, but succinct. It should tell us what the pull request is about before we even open it.

If you receive any feedback or comments, take them constructively. They're not meant to upset or insult you, only to ensure that the quality and stability of Gargoyle only improves!

---

> Contribution guidelines lifted from the atom project.
