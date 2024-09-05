# Coding style guide

## pre-commit

Lightweight automatic checks and in-place corrections are performed before commiting changes if you run:

```
$ pip install pre-commit
$ pre-commit install
pre-commit installed at .git/hooks/pre-commit
```

For more information about pre-commit, please refer to ![official website](https://pre-commit.com/).


## Guidelines on C/C++ files

Currently, pre-commit will check the code with clang-format. In addition to that, this project has additional conventions:


### Functions definition order

 1. Avoid function prototypes if possible.
 2. Functions (method) must be sorted as follows
    A. Local (static) functions, and among them sorted by by class hierarchy order
    B. Global (not API, but global among the library).
    C. Public API functions.

**Example 1**

The following example shows the functions order in case 2(A).

This is the hierarchy structure of `gstwebcanvassrc`.
```
GObject
 +----GInitiallyUnowned
       +----GstObject
             +----GstElement
                   +----GstBaseSrc
                         +----GstPushSrc
                               +----GstWebCanvasSrc
```

So, in the *.c* file "methods" of GObject must be at the end of the document,
then GstElement "GstObject", then GstElement "methods", then GstElement "methods",
then GstBaseSrc "methods", then GstPushSrc "methods" and finally
GstWebCanvasSrc methods, like the following example shows:

```
static void
gst_web_canvas_src_init (GstWebCanvasSrc *self)
{
  /* ... */
}

static void
gst_web_canvas_src_create_internal (GstWebCanvasSrc *self, GstBuffer **buffer)
{
  /* ... */
}

static GstFlowReturn
gst_web_canvas_src_create (GstPushSrc *psrc, GstBuffer **buffer)
{
  /* ... */
}

static void
gst_web_canvas_src_set_property (
    GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  /* ... */
}

static void
gst_web_canvas_src_class_init (GstWebCanvasSrcClass *klass)
{
  /* ... */
  gobject_class->set_property = gst_web_canvas_src_set_property;
  gstpushsrc_class->create = gst_web_canvas_src_create;
}
```

**Example 2**

The following example shows the functions order in case 2 (A and B):

Suppose the implementation of an hipotheticaly GstWebDOM abstract class
library (intented to be used by multiple elements):

```
// Example of 2(A)
static void
gst_web_dom_base_init (GstWebDOMBase *self)
{
  /* ... */
}

// Example of 2(A)
static void
gst_web_dom_base_class_init (GstWebDOMBase *klass)
{
  /* ... */
}

// Example of 2(B): Global internal function used by multiple files in gst-libs/gstwebdom/*.c, but not exposed.
void
gst_web_dom_base_element_parse_common (GstWebDOMBase *self)
{
  /* ... */
}

// Example of 2(C): Public API.
val *
gst_web_dom_base_get_element (GstWebDOMBase *self)
{
  /* ... */
  return val;
}
```

### Logging macros

GStreamer logging macros are explained in [official documentation](https://gstreamer.freedesktop.org/documentation/tutorials/basic/debugging-tools.html?gi-language=c#the-debug-log).

In addition, consider:
1. Use `GST_[ERROR/WARNING/...]_OBJECT` instead of `GST_[ERROR/WARNING/...]` if creating a new GstObject derivative.
2. If some information must be shown repeatedly (i.e. per buffer), use `GST_LOG` or `GST_LOG_OBJECT`.
3. If in the code there is an error and it is non-recoverable, then use  `GST_ERROR` or `GST_ERROR_OBJECT`.
4. If in the code there is an error and it is recoverable, use `GST_WARNING` or `GST_WARNING_OBJECT`.
5. Use descriptive messages for actions:
   a. **Before an Action**: Log the action you are about to perform. This prepares anyone reading the logs to understand what is coming next.
      Example: `GST_INFO("Creating buffer");` or `GST_INFO("Initializing decoder");`.
   b. **After an Action**: Log the result or outcome of the action. This confirms what happened and provides useful context, such as object addresses or results.
      Example: `GST_INFO("Created buffer %p", buffer);` or `GST_INFO("Decoder initialized");`.
   c. **Error Handling**: Clearly log when an error occurs and explain what went wrong.
      Example: `GST_ERROR("Failed to create buffer %p, no memory available.");`.

### Define macros

1. `#define` macros must be always at start of the document.
2. If really needed to `#define` a macro in the middle of the document, always undefine it.

### GType definition

1. Prefer `G_DEFINE_TYPE*`, `G_DEFINE_FINAL_TYPE*` or `G_DEFINE_ABSTRACT_TYPE*` macros over defining the GObject definition boilerplate.
2. In "methods" definition use `self` as argument name, i.e.
   ```
   static void
   gst_web_canvas_src_init (GstWebCanvasSrc *self)
   {
      /* ... */
   }
   ```

### Header files

Use header files if defined functions will be reusable within the project or by external libraries. For example, if defining the `gstwebexamplesrc.c` element file, avoid creating `gstwebexamplesrc.h` if not needed.


## Guidelines on .build, .js, .html, .recipe and other file formats

Currently, pre-commit will check the code with clang-format. In addition to that no guidelines are defined for now.


## Guidelines on sample files

Follow this pattern for each sample:

```
├── foo
│   ├── gstfoo-example.c
│   ├── html # If needed
│   │   ├── index.html
│   │   └── scripts # If needed
│   │       └── index.js
│   └── meson.build
```

For each `gst[file]-example.c` executable, add the following deubg category (if logging needed):
```
GST_DEBUG_CATEGORY_STATIC (example_dbg);
#define GST_CAT_DEFAULT example_dbg
```

## Commit messages

No specific guidelines defined here, but take inspiration of https://www.conventionalcommits.org/en/v1.0.0/
