import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

sns.set_style("whitegrid")

# load data from csv
df = pd.read_csv("./complete.csv")

# show first five entries
print(df.head(5))

# box plot
bp = sns.catplot(x="distance", y="mbit/s", kind="box", hue="protocol", data=df)
bp.set(xlabel="Abstand [m]", ylabel="Durchsatz [mbit/s]", title="Durchsatzmessung über 60 Sekunden")

# line plot
ln = sns.relplot(
    data=df,
    x="sec",
    y="mbit/s",
    hue="protocol",
    style="distance",
    kind="line",
    markers=True   
)
ln.set(xlabel="Zeit [s]", ylabel="Durchsatz [mbit/s]", title="Durchsatzmessung über 60 Sekunden")
# bar plot
bar = sns.catplot(x="distance", y="mbit/s", hue="protocol", kind="bar", data=df)
bar.set(xlabel="Abstand [m]", ylabel="Durchsatz [mbit/s]", title="Durchsatzmessung über 60 Sekunden")

plt.show()