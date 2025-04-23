# -*- Mode: Python -*- vi:si:et:sw=4:sts=4:ts=4:syntax=python
import inspect
import os
import sys
import cerbero

def recipes_path():
    return os.path.join(os.path.dirname(cerbero.__file__), '..', 'cerbero-share', 'recipes')

def import_recipe(recipe_name, class_name='Recipe'):
    file = os.path.join(recipes_path(), recipe_name)

    '''
    Mechanism to load a recipe from other file in order to inherit from it
    @file The path where the .recipe file is
    @class_name The recipe to return, by default 'Recipe'
    '''
    upframe = inspect.stack()[1].frame
    new_globals = upframe.f_globals.copy()
    new_globals['__file__'] = file
    exec(open(file).read(), new_globals)
    recipe_class = new_globals[class_name]
    # Cerbero checks for the __module__ being 'builtins' in order to load it again
    recipe_class.__module__ = None
    return recipe_class
