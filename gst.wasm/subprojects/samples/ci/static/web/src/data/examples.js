const exampleModules = import.meta.glob('../examples/*.js', { eager: true });

const compareExamples = (left, right) => {
  if (left.order !== right.order) {
    return left.order - right.order;
  }

  return left.pageName.localeCompare(right.pageName);
};

export const examplesData = Object.values(exampleModules)
  .flatMap(({ default: example }) => Array.isArray(example) ? example : [example])
  .filter((example) => example?.id)
  .sort(compareExamples);

export const examplesById = Object.fromEntries(
  examplesData.map((example) => [example.id, example]),
);

export function getExampleHash(exampleId) {
  return `#/examples/${exampleId}`;
}

export function getExampleIdFromHash(hash) {
  return hash.startsWith('#/examples/')
    ? hash.slice('#/examples/'.length)
    : null;
}
