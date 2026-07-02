exports.handler = async () => ({
  statusCode: 200,
  headers: { "content-type": "application/json" },
  body: JSON.stringify({
    ok: true,
    message: "Placeholder manager Lambda. Deploy the packaged manager API to enable controls."
  })
});
