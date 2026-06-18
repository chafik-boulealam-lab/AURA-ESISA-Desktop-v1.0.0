#include <stdio.h>
#include <gtk/gtk.h>

int main(int argc, char *argv[]) {
    FILE *f = fopen("test_gtk_output.txt", "w");
    fprintf(f, "Starting GTK test\n");
    fflush(f);
    
    if (!gtk_init_check(&argc, &argv)) {
        fprintf(f, "GTK init failed\n");
        fclose(f);
        return 1;
    }
    
    fprintf(f, "GTK init succeeded\n");
    fclose(f);
    return 0;
}
