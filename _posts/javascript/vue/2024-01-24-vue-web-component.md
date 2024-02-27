---
title: "使用 Vue 创建 Web Component"
date: 2024-01-24 19:00:00
tags: javascript vue
---

# 准备

安装 vue 命令行工具:

```shell
> yarn global add @vue/cli
```

生成一个空白仓库:

```shell
> vue create json-viewer
```

可以使用的选项如下(参考, 根据自己的需要调整):

<!--more-->

{% raw %}

```text
Vue CLI v5.0.8
? Please pick a preset: Manually select features
? Check the features needed for your project: Babel, TS, CSS Pre-processors, Linter
? Choose a version of Vue.js that you want to start the project with 2.x
? Use class-style component syntax? No
? Use Babel alongside TypeScript (required for modern mode, auto-detected polyfills, transpiling JSX)? No
? Pick a CSS pre-processor (PostCSS, Autoprefixer and CSS Modules are supported by default): Sass/SCSS (with dart-sass)
? Pick a linter / formatter config: Prettier
? Pick additional lint features: Lint on save
? Where do you prefer placing config for Babel, ESLint, etc.? In dedicated config files
? Save this as a preset for future projects? No
```

# 开发

## 功能开发

功能开发可以像普通的 vue 应用一样进行.

删除 `HelloWorld` 组件, 并创建一个叫做 `TwoCounter` 的组件, 这个组件引用另一个叫做 `BasicCounter` 的组件.

两个文件内容分别如下:

BasicCounter.vue

```vue
<template>
  <div class="basic-counter">
    <h1>Counter - {{ index }}</h1>
    <div class="counter">
      <p>Cuttent Value is {{ counter }}</p>
      <button @click="increment">Increment</button>
      <button @click="decrement">Decrement</button>
    </div>
  </div>
</template>

<script lang="ts">
import Vue from "vue";

export default Vue.extend({
  name: "BasicCounter",
  props: {
    index: String,
  },
  data: function () {
    return {
      counter: 0,
    };
  },
  methods: {
    increment: function () {
      this.counter = this.counter + 1;
    },
    decrement: function () {
      this.counter = this.counter - 1;
    },
  },
});
</script>

<!-- Add "scoped" attribute to limit CSS to this component only -->
<style scoped lang="scss">
.basic-counter {
  display: flex;
  flex-direction: column;

  h3 {
    margin: 40px 0 0;
  }
  .counter {
  }
}
</style>
```

TwoCounter.vue

```vue
<template>
  <div class="two-counter">
    <h1>{{ title }}</h1>
    <div class="counters">
      <BasicCounter index="1" />
      <BasicCounter index="2" />
    </div>
  </div>
</template>

<script lang="ts">
import Vue from "vue";
import BasicCounter from "./BasicCounter.vue";

export default Vue.extend({
  name: "TwoCounter",
  components: {
    BasicCounter,
  },
  props: {
    title: String,
  },
});
</script>

<!-- Add "scoped" attribute to limit CSS to this component only -->
<style scoped lang="scss">
.two-counter {
  width: 800px;
  .counters {
    display: flex;
    flex-direction: row;
    justify-content: space-around;
  }
}
</style>
```

写好之后, 使用以下命令编译成 Web Component:

```shell
> yarn run build -- --target wc --name two-counter --inline-vue src/components/TwoCounter.vue
```

上面的命令会在 dist 里面生成一个叫做 `two-counter.js` 的文件, 可以直接在 html 里面引用它:

```html
<!DOCTYPE html>
<html lang="">
  <head>
    <meta charset="utf-8" />
    <title>two-counter</title>
    <script src="./two-counter.js"></script>
  </head>
  <body>
    <two-counter></two-counter>
  </body>
</html>
```

最后的运行效果如下:

![](https://qiniu.iuwei.fun/blog/front-end/vue/two-counter-web-component.jpg)

## Vue 3

如果你用的是 Vue 3, 目前它对 Web Component 的支持有限, 需要使用一种变通的方式来生成.

创建一个包装性质的代码片段:

```javascript
import { createApp } from "vue";

import App from "./App.vue";

class CustomElement extends HTMLElement {
  constructor() {
    super();
  }

  connectedCallback() {
    const options = typeof App === "function" ? App.options : App;
    const propsList = Array.isArray(options.props)
      ? options.props
      : Object.keys(options.props || {});

    const props = {};
    // Validate, if all props are present
    for (const prop of propsList) {
      const propValue =
        process.env.NODE_ENV === "development"
          ? process.env[`VUE_APP_${prop.toUpperCase()}`]
          : this.attributes.getNamedItem(prop)?.value;

      if (!propValue) {
        console.error(`Missing attribute ${prop}`);
        return;
      }

      props[prop] = propValue;
    }

    const app = createApp(App, props);

    const wrapper = document.createElement("div");
    app.mount(wrapper);

    this.appendChild(wrapper.children[0]);
  }
}

window.customElements.define("two-counter", CustomElement);
```

```shell
> yarn run build -- --target lib --name two-counter --inline-vue src/two-counter.js
```

{% endraw %}
