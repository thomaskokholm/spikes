# Template for modern WEB dev

The goal for this spike is making a combination of E6 modules (SystemJS), React and Typescript.

Making new projects without ES6 modules just makes sense now a days, and using jspm as
manager sounds promising, so this need to be tried out.

React sounds interesting, and using it together with typescript and tsx is just too tempting.

jspm offers both packing and live trans coding, and works nicely and would make future web development much more smooth, as code now loads in the browser as if it was plain JS files.

## setup

To setup this framework the system need the following tools

 * tsd

   Handle typescript typings download and version syncing

 * Typescript

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