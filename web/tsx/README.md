# Template for modern WEB dev

The goal for this spike is making a combination of E6 modules, react, bootstrap and typescript.

Making new projects without ES6 modules just makes sense now a days, and using jspm as
manager sounds promising, so this need to be tried out.

React sounds interesting, and using it together with typescript is just too tempting.

jspm offers both packing and trans coding, and both would be extremely helpful for future projects.

Many nice frameworks have been made available lately, but bootstrap remain the most
used, and as this template is not about css and UI design, this seemed like a sensible
default.

## setup

To setup this framework the system need the following tools

 * tsd

   Handle typescript typings download and version syncing

 * typescript

   This is a typescript project, so typescript my be needed. Note the if you only use browser transpile this is not needed, but you editor or bundling may need this.

 * jspm

   The ES6 package manager primary for SystemJS package management for our project

 * npm

   The fallback package manager and probably the seed for the two other tools

When these are provided, the following commands need to be executed

```bash
  # tsd update
  # jspm update
```

After this, you just need you browser to point to this dir, and you then have a fully (but boring) transpiled react / typescript setup.