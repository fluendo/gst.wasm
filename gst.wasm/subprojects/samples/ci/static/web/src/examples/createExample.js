export function createExample({
  id,
  order,
  title,
  descriptionContent = null,
  streamUrl = null,
  executableName = `/${id}-example/${id}-example.js`,
  code = `/samples/code/${id}-example.c`,
}) {
  return {
    id,
    order,
    pageName: title,
    descriptionTitle: title,
    descriptionContent,
    streamUrl,
    executableName,
    code,
  };
}
